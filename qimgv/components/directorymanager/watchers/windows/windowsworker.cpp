#include "windowsworker.h"

WindowsWorker::WindowsWorker() : WatcherWorker() {

}

void WindowsWorker::setDirectoryHandle(HANDLE hDir) {
    //qDebug() << "setHandle" << this->hDir << " -> " << hDir;
    freeHandle();
    this->hDir = hDir;
}

void WindowsWorker::freeHandle() {
    if (this->hDir != nullptr && this->hDir != INVALID_HANDLE_VALUE) {
        CancelIoEx(this->hDir, NULL);
        CloseHandle(this->hDir);
        this->hDir = nullptr;
    }
}

void WindowsWorker::run() {
    isRunning = true;
    DWORD error = 0;
    bool bPending = false;
    DWORD dwBytes = 0;
    
    // 修复：使用 C++11 值初始化语法，避免 GCC 15 的 -Wmissing-field-initializers 警告
    OVERLAPPED ovl{};
    std::vector<BYTE> buffer(1024 * 64);

    ovl.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (!ovl.hEvent) {
        qDebug() << "[WindowsWorker] CreateEvent failed, error:" << GetLastError();
        return;
    }

    ::ResetEvent(ovl.hEvent);

    while (isRunning) {
        //qDebug() << "_1";
        bPending = ReadDirectoryChangesW(hDir,
                                         &buffer[0],
                                         static_cast<DWORD>(buffer.size()),
                                         FALSE,
                                         FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                         &dwBytes,
                                         &ovl,
                                         nullptr);
        //qDebug() << "_2";
        if (!bPending) {
            error = GetLastError();
            if (error == ERROR_IO_INCOMPLETE) {
                qDebug() << "ERROR_IO_INCOMPLETE";
                continue;
            }
            // 其他错误情况下也继续循环，避免死锁
            qDebug() << "[WindowsWorker] ReadDirectoryChangesW failed, error:" << error;
        }
        //qDebug() << "_3";
        bool WAIT = FALSE;  // 使用 FALSE 而非 false，与 Windows API 保持一致
        if (GetOverlappedResult(hDir, &ovl, &dwBytes, WAIT)) {
            bPending = false;
            if (dwBytes != 0) {
                FILE_NOTIFY_INFORMATION *fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
                do {
                    if (fni->Action != 0) {
                        emit notifyEvent(fni);
                    }
                    if (fni->NextEntryOffset == 0)
                        break;
                    fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<PCHAR>(fni) + fni->NextEntryOffset);
                } while (true);
            }
        }
        Sleep(POLL_RATE_MS);
    }
    
    // 清理：关闭事件句柄，避免资源泄漏
    if (ovl.hEvent != nullptr && ovl.hEvent != INVALID_HANDLE_VALUE) {
        ::CloseHandle(ovl.hEvent);
        ovl.hEvent = nullptr;
    }
}