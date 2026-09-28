// windowsworker.cpp
#include "windowsworker.h"
#include "windowswatcher_p.h"
#include <cstddef>
#include <QSet>
#include <QThread>

namespace {
constexpr qsizetype kInitialBufferSize = 131072;            // 128KB
constexpr qsizetype kMaxBufferSize = static_cast<qsizetype>(16) * 1024 * 1024;      // 16MB，防止无限膨胀
constexpr qsizetype kNetworkBufferSize = 65536;             // 64KB，网络目录(SMB)缓冲上限
constexpr qsizetype kMaxFileNameLength = 4096;              // 单条文件名上限（WCHAR）
constexpr int kShrinkQuietReads = 4;                        // 连续多少次低利用率读取后回收缓冲
}

WindowsWorker::WindowsWorker() {
    buffer.resize(kInitialBufferSize);
}

void WindowsWorker::setRunning(bool running) {
    isRunning.store(running, std::memory_order_release);

    if (!running) {
        needsRestart.store(false, std::memory_order_release); // ⭐ 防止退出时误重启
        cancelIo();  // 中断阻塞 I/O
    }
}

void WindowsWorker::setWatchPath(const QString& path) {
    QMutexLocker locker(&pathMutex);
    pendingPath = path;
}

void WindowsWorker::cancelIo() {
    HANDLE hDir = activeHandle.load(std::memory_order_acquire);
    if (hDir != INVALID_HANDLE_VALUE) {
        CancelIoEx(hDir, nullptr);
    }
}

void WindowsWorker::requestDirectoryHandle(const QString& path) {
    bool shouldCancel = false;
    {
        QMutexLocker locker(&pathMutex);
        // 路径没变且句柄有效 → 无需重启
        if (path == pendingPath &&
            activeHandle.load(std::memory_order_acquire) != INVALID_HANDLE_VALUE) {
            return;
        }
        pendingPath = path;
        shouldCancel = (activeHandle.load(std::memory_order_acquire) != INVALID_HANDLE_VALUE);
    }
    needsRestart.store(true, std::memory_order_release);
    // 立即中断当前阻塞的 ReadDirectoryChangesW，让循环快速响应新路径
    if (shouldCancel) {
        cancelIo();
    }
}

HANDLE WindowsWorker::openDirectoryHandle(const QString& path) {
    HANDLE hDir = INVALID_HANDLE_VALUE;
    int delay = 10;  // 指数退避起始

    while (isRunning.load(std::memory_order_acquire) && hDir == INVALID_HANDLE_VALUE) {
        hDir = CreateFileW(
            reinterpret_cast<LPCWSTR>(path.utf16()),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr);

        if (hDir == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_SHARING_VIOLATION) {
                QThread::msleep(delay);
                delay = std::min(delay * 2, 200);
            } else {
                break;
            }
        }
    }

    return hDir;
}

bool WindowsWorker::ensureInitialHandle(QString &currentPath) {
    {
        QMutexLocker locker(&pathMutex);
        currentPath = pendingPath;
    }
    if(hDirectory.get() == INVALID_HANDLE_VALUE && !currentPath.isEmpty()) {
        HANDLE hDir = openDirectoryHandle(currentPath);
        if(hDir != INVALID_HANDLE_VALUE) {
            hDirectory = ScopedHandle(hDir);
            activeHandle.store(hDir, std::memory_order_release);
        }
    }
    return hDirectory.get() != INVALID_HANDLE_VALUE;
}

bool WindowsWorker::handleRestart(QString &currentPath, int &quietReads) {
    QString newPath;
    {
        QMutexLocker locker(&pathMutex);
        newPath = pendingPath;
    }
    // 先使原子句柄失效，避免 GUI 线程 cancel 到旧句柄
    activeHandle.store(INVALID_HANDLE_VALUE, std::memory_order_release);
    hDirectory.close();
    currentPath = newPath;
    HANDLE hDir = openDirectoryHandle(currentPath);
    if(hDir != INVALID_HANDLE_VALUE) {
        hDirectory = ScopedHandle(hDir);
        activeHandle.store(hDir, std::memory_order_release);
    }
    // ⭐ 新句柄/新路径，网络上限标记需重新判定，缓冲回收计数也复位
    networkBufferCapped = false;
    quietReads = 0;
    return hDirectory.get() != INVALID_HANDLE_VALUE;
}

WindowsWorker::WaitResult WindowsWorker::waitOnce(DWORD &bytesReturned) {
    constexpr DWORD notifyFilter =
        FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_ATTRIBUTES |
        FILE_NOTIFY_CHANGE_SIZE |
        FILE_NOTIFY_CHANGE_LAST_WRITE;
    bytesReturned = 0;
    if(!ReadDirectoryChangesW(hDirectory.get(), buffer.data(),
                              static_cast<DWORD>(buffer.size()), FALSE,
                              notifyFilter, &bytesReturned, nullptr, nullptr)) {
        const DWORD err = GetLastError();
        // ⭐ 被 cancel 且正在退出 → 直接结束
        if(!isRunning.load(std::memory_order_acquire))
            return WaitResult::Stop;
        // ⭐ 被 restart 打断
        if(needsRestart.load(std::memory_order_acquire))
            return WaitResult::Retry;
        // ⭐ 陈旧 cancel（句柄值复用后被误取消的新读）→ 可重试，非致命
        if(err == ERROR_OPERATION_ABORTED)
            return WaitResult::Retry;
        // ⭐ 系统内部缓冲溢出（事件已丢失，扩容应用缓冲无效）→ 直接重挂
        if(err == ERROR_NOTIFY_ENUM_DIR)
            return WaitResult::Retry;
        // ⭐ 网络目录(SMB)缓冲 >64KB 触发 ERROR_INVALID_PARAMETER → 降档至 64KB
        if(err == ERROR_INVALID_PARAMETER && buffer.size() > kNetworkBufferSize) {
            buffer.resize(kNetworkBufferSize);
            networkBufferCapped = true;
            return WaitResult::Retry;
        }
        return WaitResult::Stop;
    }
    return WaitResult::Ok;
}

QVector<NotifyEvent> WindowsWorker::parseBatch(DWORD bytesReturned) {
    auto notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(buffer.data());
    QVector<NotifyEvent> batch;
    batch.reserve(bytesReturned / 32 + 16);
    do {
        const auto len = notify->FileNameLength / sizeof(WCHAR);
        if(len > 0 && len <= kMaxFileNameLength) {
            const QString fileName(QString::fromUtf16(
                reinterpret_cast<const char16_t*>(notify->FileName),
                static_cast<qsizetype>(len)));
            batch.append(NotifyEvent{fileName, notify->Action});
        }
        if(notify->NextEntryOffset == 0)
            break;
        notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
            reinterpret_cast<std::byte*>(notify) + notify->NextEntryOffset);
    } while(true);
    return batch;
}

void WindowsWorker::dedupModified(QVector<NotifyEvent> &batch) {
    // ⭐ 批内去重：Windows 一次保存常同时产生 SIZE/LAST_WRITE 两个 MODIFY
    if(batch.size() <= 1)
        return;
    QSet<QString> seenModified;
    QVector<NotifyEvent> deduped;
    deduped.reserve(batch.size());
    for(const NotifyEvent& ev : batch) {
        if(ev.action == FILE_ACTION_MODIFIED) {
            if(seenModified.contains(ev.fileName))
                continue;
            seenModified.insert(ev.fileName);
        }
        deduped.append(ev);
    }
    if(deduped.size() != batch.size())
        batch = std::move(deduped);
}

void WindowsWorker::emitAndMaybeShrink(const QVector<NotifyEvent> &batch, DWORD bytesReturned, int &quietReads) {
    // ⭐ 批量派发，减少排队信号与事件循环次数
    if(!batch.isEmpty())
        emit notifyEvents(batch);
    // ⭐ 扩容仅用于应急；突发结束后再回收缓冲，避免常驻大内存
    if(buffer.size() > kInitialBufferSize) {
        if(static_cast<qsizetype>(bytesReturned) < buffer.size() / 2)
            ++quietReads;
        else
            quietReads = 0;
        if(quietReads >= kShrinkQuietReads) {
            buffer.resize(kInitialBufferSize);
            quietReads = 0;
        }
    }
}

void WindowsWorker::run() {
    emit started();

    QString currentPath;
    if(!ensureInitialHandle(currentPath)) {
        emit finished();
        return;
    }

    // ⭐ 缓冲回收前的连续低利用率读取计数（仅 worker 线程访问）
    int quietReads = 0;

    while(true) {
        // ⭐ 优先检查运行状态（避免卡住）
        if(!isRunning.load(std::memory_order_acquire))
            break;
        // ⭐ restart 逻辑
        if(needsRestart.exchange(false, std::memory_order_acquire)) {
            if(!handleRestart(currentPath, quietReads))
                break;
            continue;
        }
        // ⭐ 发起阻塞调用前复查 restart 标志，缩小 TOCTOU 窗口
        if(needsRestart.load(std::memory_order_acquire))
            continue;

        DWORD bytesReturned = 0;
        const WaitResult wr = waitOnce(bytesReturned);
        if(wr == WaitResult::Stop)
            break;
        if(wr == WaitResult::Retry)
            continue;

        if(bytesReturned == 0) {
            // ⭐ 应用缓冲太小，整批事件被丢弃 → 指数扩容后重试（网络目录上限 64KB）
            const qsizetype limit = networkBufferCapped ? kNetworkBufferSize : kMaxBufferSize;
            if(buffer.size() < limit) {
                buffer.resize(std::min(buffer.size() * 2, limit));
                quietReads = 0;
            }
            continue;
        }

        QVector<NotifyEvent> batch = parseBatch(bytesReturned);
        dedupModified(batch);
        emitAndMaybeShrink(batch, bytesReturned, quietReads);
    }

    // ⭐ 确保 handle 关闭（避免悬挂）
    activeHandle.store(INVALID_HANDLE_VALUE, std::memory_order_release);
    hDirectory.close();

    emit finished();
}
