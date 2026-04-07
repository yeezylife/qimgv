// windowsworker.cpp
#include "windowsworker.h"
#include "windowswatcher_p.h"
#include <cstddef>
#include <QThread>

WindowsWorker::WindowsWorker() {
    buffer.resize(131072); // 128KB，提高高并发场景下的事件处理能力
}

void WindowsWorker::setDirectoryHandle(ScopedHandle handle) {
    hDirectory = std::move(handle);
}

void WindowsWorker::setWatchPath(const QString& path) {
    watchPath = path;
}

void WindowsWorker::requestDirectoryHandle(const QString& path) {
    watchPath = path;
    HANDLE hDir = INVALID_HANDLE_VALUE;
    
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
                QThread::msleep(50);
            } else {
                break;
            }
        }
    }

    if (hDir != INVALID_HANDLE_VALUE) {
        hDirectory = ScopedHandle(hDir);
    }
}

void WindowsWorker::run() {
    emit started();
    // 如果没有预先设置句柄，则尝试在 run 中请求
    if (!hDirectory && !watchPath.isEmpty()) {
        requestDirectoryHandle(watchPath);
    }
    if (!hDirectory) {
        emit finished();
        return;
    }

    DWORD bytesReturned = 0;
    constexpr DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | 
                                   FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | 
                                   FILE_NOTIFY_CHANGE_LAST_WRITE;

    while (isRunning.load(std::memory_order_acquire) &&
           ReadDirectoryChangesW(hDirectory.get(), buffer.data(), buffer.size(), FALSE, notifyFilter, &bytesReturned, nullptr, nullptr)) {
        if (bytesReturned == 0) {
            continue;
        }

        auto notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(buffer.data());
        do {
            const auto len = notify->FileNameLength / sizeof(WCHAR);
            if (len == 0 || len > 4096) {
                break;
            }

            const QString fileName(reinterpret_cast<const QChar*>(notify->FileName), static_cast<qsizetype>(len));
            emit notifyEvent(fileName, notify->Action);

            if (notify->NextEntryOffset == 0) {
                break;
            }
            notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
                reinterpret_cast<std::byte*>(notify) + notify->NextEntryOffset);
        } while (true);
    }

    emit finished();
}