#include "thumbnailer.h"
#include "sourcecontainers/thumbnail.h"

Thumbnailer::Thumbnailer() 
    : cache(std::make_unique<ThumbnailCache>()), 
      pool(new QThreadPool(this))
{
    int threads = settings->thumbnailerThreadCount();
    int globalThreads = QThreadPool::globalInstance()->maxThreadCount();
    if(threads > globalThreads)
        threads = globalThreads;
    pool->setMaxThreadCount(threads);
}

Thumbnailer::~Thumbnailer() {
    pool->clear();
    pool->waitForDone();
}

void Thumbnailer::waitForDone() {
    pool->waitForDone();
}

void Thumbnailer::clearTasks() {
    pool->clear();
}

// 同步获取：直接返回空，调用者通常会用默认图标代替
std::shared_ptr<Thumbnail> Thumbnailer::getThumbnail(const QString &filePath, int size) {
    Q_UNUSED(filePath);
    Q_UNUSED(size);
    return nullptr;
}

// 异步获取：直接发射"空"信号，告知调用者任务已结束（无需生成）
// 这样可以防止调用者因为收不到信号而反复请求
void Thumbnailer::getThumbnailAsync(const QString &path, int size, bool crop, bool force) {
    Q_UNUSED(crop);
    Q_UNUSED(force);
    
    // 关键修改：直接发射信号，返回一个空的 thumbnail
    // 这会让调用者知道："这个任务处理完了，但没有图"
    // 调用者通常会因此停止请求，并显示默认图标
    emit thumbnailReady(nullptr, path);
}

void Thumbnailer::startThumbnailerThread(const QString &filePath, int size, bool crop, bool force) {
    Q_UNUSED(filePath);
    Q_UNUSED(size);
    Q_UNUSED(crop);
    Q_UNUSED(force);
    // 已禁止启动线程
    return;
}

void Thumbnailer::onTaskStart(const QString &filePath, int size) {
    // 因为线程不会启动，这个函数理论上不会被调用
    runningTasks.insert(filePath, size);
}

void Thumbnailer::onTaskEnd(const std::shared_ptr<Thumbnail> &thumbnail, const QString &filePath) {
    // 因为线程不会启动，这个函数理论上不会被调用
    if(thumbnail) {
        runningTasks.remove(filePath, thumbnail->size());
    }
    emit thumbnailReady(thumbnail, filePath);
}