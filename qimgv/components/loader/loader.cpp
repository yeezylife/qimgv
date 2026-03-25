#include "loader.h"
#include <QThread>
#include <QCoreApplication>

Loader::Loader() {
    pool = new QThreadPool(this);

    // 🚀 自适应线程数（比固定2强很多）
    int ideal = QThread::idealThreadCount();
    pool->setMaxThreadCount(std::max(2, ideal > 1 ? ideal - 1 : 1));

    // 🚀 减少 QHash rehash
    tasks.reserve(32);
}

Loader::~Loader() {
    clearTasks();
}

void Loader::clearTasks() {
    clearPool();

    // 🚀 不再 busy-wait + processEvents
    pool->waitForDone();

    // 理论上 tasks 应该已经清空，但做兜底
    while (!tasks.isEmpty()) {
        auto it = tasks.begin();
        delete it.value();
        it = tasks.erase(it);
    }
}

bool Loader::isBusy() const {
    return !tasks.isEmpty();
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
    if (tasks.contains(path)) {
        return;
    }

    auto *runnable = new LoaderRunnable(path);
    runnable->setAutoDelete(false);

    tasks.insert(path, runnable);

    connect(runnable, &LoaderRunnable::finished,
            this, &Loader::onLoadFinished,
            Qt::UniqueConnection);

    pool->start(runnable, priority);
}

void Loader::onLoadFinished(const std::shared_ptr<Image> &image, const QString &path) {
    auto it = tasks.find(path);
    if (it != tasks.end()) {
        auto *task = it.value();
        tasks.erase(it);
        delete task;
    }

    // 🚀 shared_ptr 直接转发（减少不必要操作）
    if (!image)
        emit loadFailed(path);
    else
        emit loadFinished(image, path);
}

void Loader::clearPool() {
    for (auto it = tasks.begin(); it != tasks.end(); ) {
        LoaderRunnable *runnable = it.value();

        // 🚀 tryTake 成功才删除
        if (pool->tryTake(runnable)) {
            delete runnable;
            it = tasks.erase(it);
        } else {
            ++it;
        }
    }
}