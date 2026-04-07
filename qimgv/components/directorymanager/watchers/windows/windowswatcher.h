#ifndef WINDOWSWATCHER_H
#define WINDOWSWATCHER_H

#include "../directorywatcher.h"

class WindowsWatcherPrivate;

class WindowsWatcher : public DirectoryWatcher
{
    Q_OBJECT

public:
    explicit WindowsWatcher(QObject* parent = nullptr);
    void setWatchPath(const QString& path) override;
    void requestWatchPath(const QString& path) override;
    void cancelIo();  // 中断阻塞中的 ReadDirectoryChangesW

private:
    Q_DECLARE_PRIVATE(WindowsWatcher)
};

#endif // WINDOWSWATCHER_H