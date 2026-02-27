#include "thumbnailer.h"

Thumbnailer::Thumbnailer() 
    : cache(std::make_unique<ThumbnailCache>()), // 优化：使用 make_unique 初始化智能指针
      pool(new QThreadPool(this))
{
    // 保留线程池配置逻辑，虽然此时已无任务，但保持对象状态完整性
    int threads = settings->thumbnailerThreadCount();
    int globalThreads = QThreadPool::globalInstance()->maxThreadCount();
    if(threads > globalThreads)
        threads = globalThreads;
    pool->setMaxThreadCount(threads);
}

Thumbnailer::~Thumbnailer() {
    pool->clear();
    pool->waitForDone();
    // cache 不再需要手动 delete，智能指针会自动处理
}

void Thumbnailer::waitForDone() {
    pool->waitForDone();
}

void Thumbnailer::clearTasks() {
    pool->clear();
}

// 【修改】直接返回 nullptr，禁止同步生成缩略图
std::shared_ptr<Thumbnail> Thumbnailer::getThumbnail(const QString &filePath, int size) {
    Q_UNUSED(filePath);
    Q_UNUSED(size);
    return nullptr;
}

// 【修改】直接返回，禁止异步生成缩略图
void Thumbnailer::getThumbnailAsync(const QString &path, int size, bool crop, bool force) {
    Q_UNUSED(path);
    Q_UNUSED(size);
    Q_UNUSED(crop);
    Q_UNUSED(force);
    return;
}

void Thumbnailer::startThumbnailerThread(const QString &filePath, int size, bool crop, bool force) {
    // 该函数理论上不会被调用了（因为上层入口已拦截），但为了保险起见也保留拦截逻辑
    Q_UNUSED(filePath);
    Q_UNUSED(size);
    Q_UNUSED(crop);
    Q_UNUSED(force);
    return;
    
    /* 原逻辑保留供参考
    auto runnable = new ThumbnailerRunnable(settings->useThumbnailCache() ? cache.get() : nullptr, filePath, size, crop, force);
    connect(runnable, &ThumbnailerRunnable::taskStart, this, &Thumbnailer::onTaskStart);
    connect(runnable, &ThumbnailerRunnable::taskEnd, this, &Thumbnailer::onTaskEnd);
    runnable->setAutoDelete(true);
    pool->start(runnable);
    */
}

void Thumbnailer::onTaskStart(const QString &filePath, int size) {
    runningTasks.insert(filePath, size);
}

void Thumbnailer::onTaskEnd(std::shared_ptr<Thumbnail> thumbnail, const QString &filePath) {
    // 注意：如果 thumbnail 为空，需防止崩溃
    if(thumbnail) {
        runningTasks.remove(filePath, thumbnail->size());
    }
    emit thumbnailReady(thumbnail, filePath);
}
