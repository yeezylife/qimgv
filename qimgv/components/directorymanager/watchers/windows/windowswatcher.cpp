// windowswatcher.cpp
#include "windowswatcher_p.h"
#include "windowsworker.h"

WindowsWatcherPrivate::WindowsWatcherPrivate(WindowsWatcher* qq)
    : DirectoryWatcherPrivate(static_cast<DirectoryWatcher*>(qq), new WindowsWorker())
{
    auto windowsWorker = static_cast<WindowsWorker*>(worker.data());
    qRegisterMetaType<DWORD>("DWORD");
    connect(windowsWorker, &WindowsWorker::notifyEvent, this, &WindowsWatcherPrivate::dispatchNotify);
}

void WindowsWatcherPrivate::dispatchNotify(const QString& fileName, DWORD action)
{
    Q_Q(WindowsWatcher);

    if (fileName.isEmpty() || fileName.length() > 4096) {
        return;
    }

    switch (action) {
    case FILE_ACTION_ADDED:
        emit q->fileCreated(fileName);
        break;
    case FILE_ACTION_MODIFIED:
        emit q->fileModified(fileName);
        break;
    case FILE_ACTION_REMOVED:
        emit q->fileDeleted(fileName);
        break;
    case FILE_ACTION_RENAMED_NEW_NAME:
        emit q->fileRenamed(oldFileName, fileName);
        break;
    case FILE_ACTION_RENAMED_OLD_NAME:
        oldFileName = fileName;
        break;
    default:
        break;
    }
}

WindowsWatcher::WindowsWatcher(QObject* parent)
    : DirectoryWatcher(new WindowsWatcherPrivate(this))
{
    Q_D(WindowsWatcher);
    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    connect(windowsWorker, &WindowsWorker::finished, d->workerThread.data(), &QThread::quit);
    connect(windowsWorker, &WindowsWorker::started, this, &WindowsWatcher::observingStarted);
    connect(windowsWorker, &WindowsWorker::finished, this, &WindowsWatcher::observingStopped);
}

void WindowsWatcher::setWatchPath(const QString &path)
{
    Q_D(WindowsWatcher);
    DirectoryWatcher::setWatchPath(path);
    d->currentDirectory = path;
    
    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    if (windowsWorker) {
        windowsWorker->setWatchPath(path);
    }
}

void WindowsWatcher::stopObserving()
{
    Q_D(WindowsWatcher);
    if (!d->workerThread || !d->workerThread->isRunning())
        return;

    // 先中断阻塞的 I/O，让工作线程能快速退出
    cancelIo();
    DirectoryWatcher::stopObserving();
}

void WindowsWatcher::requestWatchPath(const QString& path)
{
    Q_D(WindowsWatcher);
    DirectoryWatcher::setWatchPath(path);
    
    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    if (windowsWorker) {
        // 直接调用而非 invokeMethod，避免依赖事件队列
        windowsWorker->requestDirectoryHandle(path);
    }
}

void WindowsWatcher::cancelIo()
{
    Q_D(WindowsWatcher);
    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    if (windowsWorker) {
        // 直接调用而非 invokeMethod
        windowsWorker->cancelIo();
    }
}
