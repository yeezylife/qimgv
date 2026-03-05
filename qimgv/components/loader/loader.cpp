#include "loader.h"
#include <QThread>

Loader::Loader() {
    pool = new QThreadPool(this);
    // 根据系统CPU核心数动态调整线程数，最少2个，最多4个
    int maxThreads = qBound(2, QThread::idealThreadCount(), 4);
    pool->setMaxThreadCount(maxThreads);
}

void Loader::clearTasks() {
    clearPool();
    pool->waitForDone();
}

bool Loader::isBusy() const {
    return (tasks.count() != 0);
}

bool Loader::isLoading(QString path) {
    return tasks.contains(path);
}

std::shared_ptr<Image> Loader::load(QString path) {
    return ImageFactory::createImage(path);
}

// clears all buffered tasks before loading
void Loader::loadAsyncPriority(QString path) {
    clearPool();
    doLoadAsync(path, 1);
}

void Loader::loadAsync(QString path) {
    doLoadAsync(path, 0);
}

void Loader::doLoadAsync(QString path, int priority) {
    if(tasks.contains(path)) {
        return;
    }

    auto runnable = new LoaderRunnable(path);
    runnable->setAutoDelete(false);
    tasks.insert(path, runnable);
    connect(runnable, &LoaderRunnable::finished, this, &Loader::onLoadFinished, Qt::UniqueConnection);
    pool->start(runnable, priority);
}

void Loader::onLoadFinished(std::shared_ptr<Image> image, const QString &path) {
    auto task = tasks.take(path);
    delete task;
    if(!image)
        emit loadFailed(path);
    else
        emit loadFinished(image, path);
}

void Loader::clearPool() {
    QHashIterator<QString, LoaderRunnable*> i(tasks);
    while (i.hasNext()) {
        i.next();
        if(pool->tryTake(i.value())) {
            delete tasks.take(i.key());
        }
    }
}
