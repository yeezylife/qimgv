#include "windowsworker.h"
#include "windowswatcher_p.h"

WindowsWorker::WindowsWorker() : WatcherWorker() {
    // 预分配缓冲区，避免每次重新分配
    buffer.resize(65536);  // 64KB 缓冲区
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

    uint bytesReturned = 0;
    
    while (true) {
        // 使用持续有效的缓冲区
        BOOL result = ReadDirectoryChangesW(
            hDirectory,
            buffer.data(),
            buffer.size(),
            FALSE,  // 不监视子目录
            FILE_NOTIFY_CHANGE_FILE_NAME | 
            FILE_NOTIFY_CHANGE_DIR_NAME | 
            FILE_NOTIFY_CHANGE_ATTRIBUTES | 
            FILE_NOTIFY_CHANGE_SIZE | 
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            nullptr,
            nullptr);
        
        if (!result) {
            qDebug() << "ReadDirectoryChangesW failed:" << lastError();
            break;
        }
        
        if (bytesReturned == 0) {
            continue;
        }
        
        // 解析 FILE_NOTIFY_INFORMATION 链表
        PFILE_NOTIFY_INFORMATION notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(buffer.data());
        
        do {
            // 在当前线程中安全地转换文件名
            int len = notify->FileNameLength / sizeof(WCHAR);
            
            // 安全检查：防止异常长度
            if (len <= 0 || len > 2048) {
                qDebug() << "Invalid FileNameLength:" << notify->FileNameLength;
                break;
            }
            
            // 在发送信号前转换为 QString（数据复制到这里就安全了）
            QString fileName = QString::fromWCharArray(
                reinterpret_cast<wchar_t*>(notify->FileName), 
                len);
            
            uint action = notify->Action;
            
            // 发送安全的字符串副本，而不是原始指针
            emit notifyEvent(fileName, action);
            
            // 移动到下一个通知结构
            if (notify->NextEntryOffset == 0) {
                break;
            }
            notify = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
                reinterpret_cast<char*>(notify) + notify->NextEntryOffset);
                
        } while (true);
    }
    
    emit finished();
}
