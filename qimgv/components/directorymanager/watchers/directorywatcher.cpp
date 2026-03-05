#include "directorywatcher_p.h"

#if defined(__linux__) || defined(__FreeBSD__)
#include "linux/linuxwatcher.h"
#elif defined(_WIN32)
#include "windows/windowswatcher.h"
#elif defined(__APPLE__)
#include "dummywatcher.h"
#elif defined(__unix__)
#include "dummywatcher.h"
#else
#include "dummywatcher.h"
#endif

DirectoryWatcherPrivate::DirectoryWatcherPrivate(DirectoryWatcher* qq, WatcherWorker* w)
    : QObject(nullptr)
    , q_ptr(qq)
    , worker(w)
    , workerThread(new QThread())
{
}

DirectoryWatcher::DirectoryWatcher(DirectoryWatcherPrivate* ptr)
    : QObject(nullptr)
    , d_ptr(ptr)
{
}

DirectoryWatcher::~DirectoryWatcher()
{
    stopObserving();
    delete d_ptr;
}

DirectoryWatcher* DirectoryWatcher::newInstance()
{
#if defined(__linux__) || defined(__FreeBSD__)
    return new LinuxWatcher();
#elif defined(_WIN32)
    return new WindowsWatcher();
#elif defined(__APPLE__)
    return new DummyWatcher();
#elif defined(__unix__)
    return new DummyWatcher();
#else
    return new DummyWatcher();
#endif
}

void DirectoryWatcher::setWatchPath(const QString& path)
{
    Q_D(DirectoryWatcher);
    d->currentDirectory = path;
}

QString DirectoryWatcher::watchPath() const
{
    Q_D(const DirectoryWatcher);
    return d->currentDirectory;
}

bool DirectoryWatcher::isObserving() const
{
    Q_D(const DirectoryWatcher);
    return d->workerThread->isRunning();
}

void DirectoryWatcher::observe()
{
    Q_D(DirectoryWatcher);
    if (!isObserving()) {
        d->worker->setRunning(true);
        d->workerThread->start();
    }
}

void DirectoryWatcher::stopObserving()
{
    Q_D(DirectoryWatcher);
    
    if (!d->workerThread->isRunning()) {
        return;
    }

    d->worker->setRunning(false);
    d->workerThread->quit();
    
    if (!d->workerThread->wait(1000)) {
        d->workerThread->terminate();
        d->workerThread->wait(1000);
    }
}
