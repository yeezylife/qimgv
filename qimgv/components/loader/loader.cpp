#include "loader.h"
#include <QThread>
#include <QCoreApplication> // 用于 processEvents

Loader::Loader() {
    pool = new QThreadPool(this);
    // 固定使用4个线程
    pool->setMaxThreadCount(2);
}

Loader::~Loader() {
    clearTasks(); // 确保所有任务被清理
    // 不需要手动删除 pool，Qt 对象树会自动删除
}

void Loader::clearTasks() {
    clearPool();               // 取消所有尚未开始的任务
    pool->waitForDone();       // 等待正在运行的任务完成
    
    // 安全地处理剩余的信号，避免重入问题
    while (!tasks.isEmpty()) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (!tasks.isEmpty()) {
            QThread::msleep(10); // 避免忙等待，给信号处理时间
        }
    }
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

// 修复 clearPool：先收集所有键，再尝试取消任务
void Loader::clearPool() {
    QStringList keys = tasks.keys();
    for (const QString &key : keys) {
        LoaderRunnable *runnable = tasks.value(key); // 获取任务指针
        if (pool->tryTake(runnable)) {               // 如果任务尚未开始，成功取消
            delete tasks.take(key);                   // 从哈希表中移除并删除
        }
        // 如果任务已在运行，则保留，等待其自然完成
    }
}
