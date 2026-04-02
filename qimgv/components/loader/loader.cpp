#include "loader.h"
#include <QThread>
#include <QMutableHashIterator>

Loader::Loader() {
    pool = new QThreadPool(this);
    pool->setMaxThreadCount(2);

    // 🚀 减少 QHash rehash
    tasks.reserve(32);
}

Loader::~Loader() {
    clearTasks(); 
    // 注意：这里不再调用 waitForDone()
    // QThreadPool 的析构函数会等待所有线程结束，但由于我们在 clearTasks 中
    // 已经处理了任务对象的归属权，这里可以快速安全地离开。
}

void Loader::clearTasks() {
    // 🚀 优化：非阻塞清理
    // 遍历所有任务，区分“排队中”和“运行中”分别处理
    QMutableHashIterator<QString, LoaderRunnable*> it(tasks);
    
    while (it.hasNext()) {
        it.next();
        LoaderRunnable *runnable = it.value();

        // tryTake 仅对尚未启动的任务返回 true
        if (pool->tryTake(runnable)) {
            // 任务还在队列中，直接删除对象
            delete runnable;
        } else {
            // 任务正在运行中，无法通过 tryTake 移除
            // 策略：断开信号连接，防止任务完成后触发 UI 更新
            // 并设置 autoDelete 让线程池在任务结束时自动回收内存
            runnable->disconnect();
            runnable->setAutoDelete(true);
        }
        
        // 从哈希表中移除记录
        it.remove();
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
    clearTasks(); // 清理当前任务，优先加载新任务
    doLoadAsync(path, 1);
}

void Loader::loadAsync(const QString &path) {
    doLoadAsync(path, 0);
}

void Loader::doLoadAsync(const QString &path, int priority) {
    // 🚀 优化：用 insert 返回值替代 contains + insert 双查找
    auto result = tasks.insert(path, nullptr);
    if (!result.second) {
        return; // 已在加载中
    }

    auto *runnable = new LoaderRunnable(path);
    runnable->setAutoDelete(false); // 我们手动管理内存，以便在 clearTasks 时安全删除
    
    result.first.value() = runnable;
    connect(runnable, &LoaderRunnable::finished, this, &Loader::onLoadFinished);
    
    pool->start(runnable, priority);
}

void Loader::onLoadFinished(const std::shared_ptr<Image> &image, const QString &path) {
    auto *task = tasks.take(path);
    if (task) {
        // 转发结果
        if (!image) emit loadFailed(path);
        else emit loadFinished(image, path);
        task->deleteLater(); // 安全删除，避免跨线程 delete 竞态
    }
    // 任务不在 tasks 中 (已被 clearTasks 移除/取消)
    // runnable 已在 clearTasks 中被设置为 autoDelete=true，
    // 线程池会负责回收其内存。
}
