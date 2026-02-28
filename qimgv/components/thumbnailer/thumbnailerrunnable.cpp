#include "thumbnailerrunnable.h"

// 优化：初始化列表构造
ThumbnailerRunnable::ThumbnailerRunnable(ThumbnailCache* _cache, const QString &_path, int _size, bool _crop, bool _force) :
    path(_path),
    size(_size),
    crop(_crop),
    force(_force),
    cache(_cache)
{
}

void ThumbnailerRunnable::run() {
    // 发出开始信号，保持流程一致性
    emit taskStart(path, size);
    
    // 【核心修改】直接返回空指针，不进行任何生成操作
    // 这会彻底切断图片解码和视频截帧的 CPU 消耗
    std::shared_ptr<Thumbnail> thumbnail = nullptr;
    
    // 发出结束信号，告知上层任务已完成（虽然结果是空的）
    // 这一步至关重要，能防止上层逻辑因“未收到回应”而反复重试
    emit taskEnd(thumbnail, path);
}

QString ThumbnailerRunnable::generateIdString(const QString &path, int size, bool crop) {
    // 该函数已无实际作用，保留以防万一其他地方调用
    QString queryStr = path + QString::number(size);
    if(crop)
        queryStr.append("s");
    queryStr = QString("%1").arg(QString(QCryptographicHash::hash(queryStr.toUtf8(),QCryptographicHash::Md5).toHex()));
    return queryStr;
}

std::shared_ptr<Thumbnail> ThumbnailerRunnable::generate(ThumbnailCache* cache, const QString &path, int size, bool crop, bool force) {
    // 【核心修改】同步生成接口也直接返回空，防止被直接调用时产生消耗
    Q_UNUSED(cache);
    Q_UNUSED(path);
    Q_UNUSED(size);
    Q_UNUSED(crop);
    Q_UNUSED(force);
    return nullptr;
    
    /* 原有的数百行生成逻辑已全部移除，既优化了性能，也消除了潜在的内存泄漏风险 */
}

ThumbnailerRunnable::~ThumbnailerRunnable() {
}

// 以下函数保留空实现或移除具体逻辑，因为 generate 已经不会调用它们了
std::pair<QImage*, QSize> ThumbnailerRunnable::createThumbnail(const QString &path, const char *format, int size, bool squared) {
    Q_UNUSED(path);
    Q_UNUSED(format);
    Q_UNUSED(size);
    Q_UNUSED(squared);
    return std::make_pair(nullptr, QSize());
}

std::pair<QImage*, QSize> ThumbnailerRunnable::createVideoThumbnail(const QString &path, int size, bool squared) {
    Q_UNUSED(path);
    Q_UNUSED(size);
    Q_UNUSED(squared);
    return std::make_pair(nullptr, QSize());
}
