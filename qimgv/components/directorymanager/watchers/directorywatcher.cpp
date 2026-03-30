#include "directorywatcher_p.h"

static constexpr int kThreadWaitMs = 2000;
static constexpr int kThreadWaitMsDestructor = 3000;

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

#include <QMetaObject>

DirectoryWatcherPrivate::DirectoryWatcherPrivate(DirectoryWatcher* qq, WatcherWorker* w)
    : QObject(nullptr)
    , q_ptr(qq)
    , worker(w)
    , workerThread(new QThread())
{
    if (worker && workerThread) {
        worker->moveToThread(workerThread.data());
        connect(workerThread.data(), &QThread::started, this, &DirectoryWatcherPrivate::startWorker);
        // 不再连接 finished 到 worker->deleteLater，避免与 QScopedPointer 冲突
        // 在析构函数中确保线程停止后手动 reset worker
    }
}

DirectoryWatcherPrivate::~DirectoryWatcherPrivate()
{
    // 确保线程优雅停止
    if (workerThread && workerThread->isRunning()) {
        if (worker)
            worker->setRunning(false);
        workerThread->quit();
        workerThread->wait(kThreadWaitMsDestructor);
    }
    // worker 和 workerThread 由 QScopedPointer 自动清理
}

void DirectoryWatcherPrivate::startWorker()
{
    if (!worker)
        return;
    // 调用 run() 槽函数（需将 WatcherWorker::run 声明为槽）
    QMetaObject::invokeMethod(worker.data(), "run", Qt::QueuedConnection);
}

// ==================== DirectoryWatcher 实现 ====================

DirectoryWatcher::DirectoryWatcher(DirectoryWatcherPrivate* ptr)
    : QObject(nullptr)
    , d_ptr(ptr)
{
}

DirectoryWatcher::~DirectoryWatcher()
{
    // 线程停止逻辑在 ~DirectoryWatcherPrivate() 中处理
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
    return d->workerThread && d->workerThread->isRunning();
}

void DirectoryWatcher::observe()
{
    Q_D(DirectoryWatcher);
    if (!d->workerThread || isObserving())
        return;

    d->worker->setRunning(true);
    d->workerThread->start();
    emit observingStarted();
}

void DirectoryWatcher::stopObserving()
{
    Q_D(DirectoryWatcher);

    if (!d->workerThread || !d->workerThread->isRunning())
        return;

    d->worker->setRunning(false);
    d->workerThread->quit();
    d->workerThread->wait(kThreadWaitMs);
    emit observingStopped();
}
