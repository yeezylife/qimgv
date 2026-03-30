// windowsworker.cpp
#include "windowsworker.h"
#include "windowswatcher_p.h"
#include <cstddef>

WindowsWorker::WindowsWorker() {
    buffer.resize(65536);
}

void WindowsWorker::setDirectoryHandle(HANDLE handle) {
    hDirectory = handle;
}

void WindowsWorker::run() {
    emit started();
    if (hDirectory == INVALID_HANDLE_VALUE) {
        emit finished();
        return;
    }

    DWORD bytesReturned = 0;
    constexpr DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | 
                                   FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | 
                                   FILE_NOTIFY_CHANGE_LAST_WRITE;

    while (ReadDirectoryChangesW(hDirectory, buffer.data(), buffer.size(), FALSE, notifyFilter, &bytesReturned, nullptr, nullptr)) {
        if (bytesReturned == 0) {
            continue;
        }

        auto notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(buffer.data());
        do {
            const auto len = notify->FileNameLength / sizeof(WCHAR);
            if (len == 0 || len > 4096) {
                break;
            }

            // 直接使用 QChar 构造 QString，在 Windows (UTF-16) 下零额外转换开销，性能最优
            const QString fileName(reinterpret_cast<const QChar*>(notify->FileName), static_cast<qsizetype>(len));
            emit notifyEvent(fileName, notify->Action);

            if (notify->NextEntryOffset == 0) {
                break;
            }
            // C++20: 使用 std::byte 替代 char 进行指针算术，避免违反严格别名规则并利于编译器优化
            notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
                reinterpret_cast<std::byte*>(notify) + notify->NextEntryOffset);
        } while (true);
    }

    emit finished();
}
