// windowsworker.h
#ifndef WINDOWSWORKER_H
#define WINDOWSWORKER_H

#include "../watcherworker.h"
#include <windows.h>
#include <QString>

class WindowsWorker : public WatcherWorker {
    Q_OBJECT
public:
    explicit WindowsWorker();
    void run() override;
    void setDirectoryHandle(HANDLE handle);

signals:
    void notifyEvent(const QString& fileName, DWORD action);
    void finished();
    void started();

private:
    HANDLE hDirectory = INVALID_HANDLE_VALUE;
    QByteArray buffer;
};

#endif // WINDOWSWORKER_H
