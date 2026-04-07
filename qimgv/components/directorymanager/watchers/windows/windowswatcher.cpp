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
}

void WindowsWatcher::requestWatchPath(const QString& path)
{
    Q_D(WindowsWatcher);
    DirectoryWatcher::setWatchPath(path);
    
    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    if (windowsWorker && d->workerThread->isRunning()) {
        QMetaObject::invokeMethod(windowsWorker, "requestDirectoryHandle",
                                  Qt::QueuedConnection, Q_ARG(QString, path));
    } else {
        windowsWorker->setWatchPath(path);
    }
}

void WindowsWatcher::cancelIo()
{
    Q_D(WindowsWatcher);
    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    if (windowsWorker) {
        QMetaObject::invokeMethod(windowsWorker, "cancelIo", Qt::QueuedConnection);
    }
}
