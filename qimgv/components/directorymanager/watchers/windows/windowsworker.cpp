// windowsworker.cpp
#include "windowsworker.h"
#include "windowswatcher_p.h"
#include <cstddef>
#include <QThread>

WindowsWorker::WindowsWorker() {
    buffer.resize(131072); // 128KB
}

void WindowsWorker::setRunning(bool running) {
    isRunning.store(running, std::memory_order_release);
    if (!running) {
        cancelIo();  // 中断阻塞的 I/O，让 run() 快速退出
    }
}

void WindowsWorker::setWatchPath(const QString& path) {
    QMutexLocker locker(&pathMutex);
    watchPath = path;
    pendingPath = path;
}

void WindowsWorker::cancelIo() {
    HANDLE hDir = hDirectory.get();
    if (hDir) {
        CancelIoEx(hDir, nullptr);
    }
}

void WindowsWorker::requestDirectoryHandle(const QString& path) {
    bool shouldCancel = false;
    {
        QMutexLocker locker(&pathMutex);
        if (path == pendingPath && hDirectory) return;
        pendingPath = path;
        shouldCancel = (hDirectory != nullptr);
    }
    needsRestart.store(true, std::memory_order_release);
    // 立即中断当前阻塞的 ReadDirectoryChangesW，让循环快速响应新路径
    if (shouldCancel) {
        HANDLE hDir = hDirectory.get();
        if (hDir) CancelIoEx(hDir, nullptr);
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

void WindowsWorker::run() {
    emit started();

    QString currentPath;
    {
        QMutexLocker locker(&pathMutex);
        currentPath = pendingPath.isEmpty() ? watchPath : pendingPath;
    }

    if (!hDirectory && !currentPath.isEmpty()) {
        HANDLE hDir = openDirectoryHandle(currentPath);
        if (hDir) {
            hDirectory = ScopedHandle(hDir);
        }
    }

    if (!hDirectory) {
        emit finished();
        return;
    }

    constexpr DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                   FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                                   FILE_NOTIFY_CHANGE_LAST_WRITE;

    while (isRunning.load(std::memory_order_acquire)) {
        if (needsRestart.exchange(false, std::memory_order_acquire)) {
            QString newPath;
            {
                QMutexLocker locker(&pathMutex);
                newPath = pendingPath;
            }

            hDirectory.close();

            currentPath = newPath;
            HANDLE hDir = openDirectoryHandle(currentPath);
            if (hDir) {
                hDirectory = ScopedHandle(hDir);
            }

            if (!hDirectory) {
                emit finished();
                return;
            }
            continue;
        }

        DWORD bytesReturned = 0;
        if (!ReadDirectoryChangesW(hDirectory.get(), buffer.data(), buffer.size(), FALSE,
                                    notifyFilter, &bytesReturned, nullptr, nullptr)) {
            // 被 CancelIoEx 中断 → 正常切换
            if (needsRestart.load(std::memory_order_acquire)) {
                continue;
            }
            // 真正的 I/O 错误 → 退出
            break;
        }

        if (bytesReturned == 0) continue;

        auto notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(buffer.data());
        do {
            const auto len = notify->FileNameLength / sizeof(WCHAR);
            if (len == 0 || len > 4096) break;

            const QString fileName(reinterpret_cast<const QChar*>(notify->FileName), static_cast<qsizetype>(len));
            emit notifyEvent(fileName, notify->Action);

            if (notify->NextEntryOffset == 0) break;
            notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
                reinterpret_cast<std::byte*>(notify) + notify->NextEntryOffset);
        } while (true);
    }

    emit finished();
}