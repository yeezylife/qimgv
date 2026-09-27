#include "loader.h"
#include "utils/imagefactory.h"
#include "settings.h"
#include <QMutableHashIterator>

Loader::Loader() {
    pool = new QThreadPool(this);
    pool->setMaxThreadCount(2);
    // 当前图片走独立单线程池：即使两个 preload 线程都被占用，优先级加载也能立即开始
    priorityPool = new QThreadPool(this);
    priorityPool->setMaxThreadCount(1);

    // 🚀 减少 QHash rehash
    tasks.reserve(32);
}

Loader::~Loader() {
    // 1. 取消排队中的任务（标记取消并移出 tasks，被线程池拾取后 run() 跳过解码并投递空事件）
    clearTasks();
    // 2. 等待运行中任务完成（结果事件已投递，其捕获的 self 引用维系对象存活）
    pool->waitForDone();
    priorityPool->waitForDone();
    // 3. 释放哈希引用；残留事件在 ~QObject 时被丢弃，随之释放引用并销毁对象。
    //    不能手动 delete（对象可能仍被待处理事件引用）。
    tasks.clear();
    // pool / priorityPool 作为子对象随后析构
}

void Loader::clearTasks() {
    // 单遍 O(n)：只取消"排队中"的任务，保留运行中的任务让其完成并进缓存，
    // 不再逐任务 tryTake（tryTake 每次扫描队列，任务多时为 O(n²)）。
    QMutableHashIterator<QString, std::shared_ptr<LoaderRunnable>> it(tasks);

    while (it.hasNext()) {
        it.next();
        LoaderRunnable *runnable = it.value().get();

        if (runnable->tryCancel()) {
            // CAS 成功：任务尚未启动，已被取消 → 移出哈希，run() 拾取后跳过解码
            it.remove();
        }
        // CAS 失败：任务已在运行，保留，完成后结果仍会进缓存（preload 在正常翻页时真正生效）
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
    doLoadAsync(priorityPool, path);
}

void Loader::loadAsync(const QString &path) {
    doLoadAsync(pool, path);
}

void Loader::doLoadAsync(QThreadPool *targetPool, const QString &path) {
    if (tasks.contains(path)) {
        return; // 已在加载中（含运行中的 preload，其结果会进缓存）
    }
    
    // GUI 线程快照设置，随任务带入 worker，避免解码线程访问 Settings
    auto runnable = std::make_shared<LoaderRunnable>(this, path,
        settings->memoryAllocationLimit(), settings->jxlAnimation(), settings->videoPlayback());
    runnable->setAutoDelete(false); // 生命周期由 shared_ptr 管理，QThreadPool 不得自动 delete
    runnable->self = runnable; // 排队/运行期间维系对象存活
    tasks.insert(path, runnable);
    targetPool->start(runnable.get());
}

void Loader::onLoadFinished(const std::shared_ptr<Image> &image, const QString &path) {
    if (!tasks.remove(path)) {
        return; // 已被 clearTasks 取消并移出，忽略其结果
    }
    // 事件捕获的 self 引用在事件处理完后自动释放并销毁对象，无需 deleteLater
    if (!image) emit loadFailed(path);
    else emit loadFinished(image, path);
}
