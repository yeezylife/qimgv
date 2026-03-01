#ifndef WINDOWSWATCHERWORKER_H
#define WINDOWSWATCHERWORKER_H

#include <windows.h>
#include "../watcherworker.h"
#include <QDebug>
#include <vector>

class WindowsWorker : public WatcherWorker {
    Q_OBJECT
public:
    WindowsWorker();

    void setDirectoryHandle(HANDLE hDir);
    virtual void run() override;

signals:
    void notifyEvent(PFILE_NOTIFY_INFORMATION);

private:
    HANDLE hDir = nullptr;  // 初始化为 nullptr，避免未定义行为
    uint POLL_RATE_MS = 1000;
    void freeHandle();
};

#endif // WINDOWSWATCHERWORKER_H