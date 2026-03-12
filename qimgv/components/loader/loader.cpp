#include "loader.h"
#include <QThread>
#include <QCoreApplication>

Loader::Loader() {
    pool = new QThreadPool(this);
    pool->setMaxThreadCount(2);
}

Loader::~Loader() {
    clearTasks();
}

void Loader::clearTasks() {
    clearPool();
    pool->waitForDone();
    
    while (!tasks.isEmpty()) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (!tasks.isEmpty()) {
            QThread::msleep(10);
        }
    }
}

bool Loader::isBusy() const {
    return (tasks.count() != 0);
}

bool Loader::isLoading(const QString &path) {
    return tasks.contains(path);
}

std::shared_ptr<Image> Loader::load(const QString &path) {
    return ImageFactory::createImage(path);
}

void Loader::loadAsyncPriority(const QString &path) {
    clearPool();
    doLoadAsync(path, 1);
}

void Loader::loadAsync(const QString &path) {
    doLoadAsync(path, 0);
}

void Loader::doLoadAsync(const QString &path, int priority) {
    if(tasks.contains(path)) {
        return;
    }

    auto runnable = new LoaderRunnable(path);
    runnable->setAutoDelete(false);
    tasks.insert(path, runnable);
    connect(runnable, &LoaderRunnable::finished, this, &Loader::onLoadFinished, Qt::UniqueConnection);
    pool->start(runnable, priority);
}

void Loader::onLoadFinished(const std::shared_ptr<Image> &image, const QString &path) {
    auto task = tasks.take(path);
    delete task;
    if(!image)
        emit loadFailed(path);
    else
        emit loadFinished(image, path);
}

void Loader::clearPool() {
    QStringList keys = tasks.keys();
    for (const QString &key : keys) {
        LoaderRunnable *runnable = tasks.value(key);
        if (pool->tryTake(runnable)) {
            delete tasks.take(key);
        }
    }
}