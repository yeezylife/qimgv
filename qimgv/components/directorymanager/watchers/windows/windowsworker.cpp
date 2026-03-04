#include "windowsworker.h"
#include <windows.h>
#include <winbase.h>
#include <QDebug>
#include <vector>

WindowsWorker::WindowsWorker() : WatcherWorker() {

}

void WindowsWorker::setDirectoryHandle(HANDLE hDir) {
    //qDebug() << "setHandle" << this->hDir << " -> " << hDir;
    
    // 先停止当前的监控
    isRunning = false;
    
    // 等待当前操作完成或取消
    if (this->hDir != nullptr && this->hDir != INVALID_HANDLE_VALUE) {
        // 取消所有挂起的I/O操作
        CancelIoEx(this->hDir, NULL);
        // 等待一小段时间确保操作被取消
        Sleep(100);
    }
    
    // 清理旧句柄
    freeHandle();
    
    // 设置新句柄并重新开始监控
    this->hDir = hDir;
    isRunning = true;
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
        // 检查是否需要取消当前操作
        if (!isRunning) {
            break;
        }
        
        bPending = ReadDirectoryChangesW(hDir,
                                         &buffer[0],
                                         static_cast<DWORD>(buffer.size()),
                                         FALSE,
                                         FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
                                         &dwBytes,
                                         &ovl,
                                         nullptr);
        
        if (!bPending) {
            error = GetLastError();
            if (error == ERROR_IO_INCOMPLETE) {
                qDebug() << "ERROR_IO_INCOMPLETE";
                continue;
            }
            // 如果是操作被取消的错误，这是正常的，不需要记录为错误
            if (error == ERROR_OPERATION_ABORTED) {
                // 正常的取消操作，继续循环等待下一次或退出
                continue;
            }
            // 其他错误情况下也继续循环，避免死锁
            qDebug() << "[WindowsWorker] ReadDirectoryChangesW failed, error:" << error;
        }
        
        // 如果有异步操作正在进行，等待其完成或超时
        if (bPending) {
            DWORD dwWaitResult = WaitForSingleObject(ovl.hEvent, POLL_RATE_MS);
            
            if (dwWaitResult == WAIT_TIMEOUT) {
                // 超时，检查是否需要取消操作
                if (!isRunning) {
                    // 取消异步I/O操作
                    CancelIoEx(hDir, &ovl);
                    continue;
                }
                // 继续下一次循环
                continue;
            } else if (dwWaitResult == WAIT_OBJECT_0) {
                // 操作完成，处理结果
                if (GetOverlappedResult(hDir, &ovl, &dwBytes, FALSE)) {
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
                } else {
                    error = GetLastError();
                    if (error == ERROR_OPERATION_ABORTED) {
                        // 正常的取消操作
                        continue;
                    } else {
                        qDebug() << "[WindowsWorker] GetOverlappedResult failed, error:" << error;
                    }
                }
            } else {
                // 其他等待结果，记录错误
                qDebug() << "[WindowsWorker] WaitForSingleObject failed, result:" << dwWaitResult;
            }
        } else {
            // 没有异步操作，直接睡眠
            Sleep(POLL_RATE_MS);
        }
    }
    
    // 确保所有异步操作都被取消
    if (hDir != nullptr && hDir != INVALID_HANDLE_VALUE) {
        CancelIoEx(hDir, NULL);
    }
    
    // 清理：关闭事件句柄，避免资源泄漏
    if (ovl.hEvent != nullptr && ovl.hEvent != INVALID_HANDLE_VALUE) {
        ::CloseHandle(ovl.hEvent);
        ovl.hEvent = nullptr;
    }
    
    emit finished();
}
