#ifndef WINDOWSWORKER_H
#define WINDOWSWORKER_H

#include "watcherworker.h"
#include <windows.h>
#include <QString>

class WindowsWorker : public WatcherWorker {
    Q_OBJECT
public:
    explicit WindowsWorker();
    virtual void run() override;
    
    void setDirectoryHandle(HANDLE handle);

signals:
    // 修改：传递 QString 和 DWORD 而不是原始指针
    void notifyEvent(const QString& fileName, DWORD action);
    void finished();
    void started();

private:
    HANDLE hDirectory = INVALID_HANDLE_VALUE;
    QByteArray buffer;  // 确保持续的缓冲区
};

#endif // WINDOWSWORKER_H
