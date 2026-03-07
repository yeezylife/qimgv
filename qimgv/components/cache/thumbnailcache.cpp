#include "thumbnailcache.h"

ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent)
{
    // 缩略图缓存已禁用，不执行任何初始化操作
}

void ThumbnailCache::saveThumbnail(const QImage &image, QStringView id) {
    // 缩略图保存已禁用，不执行任何操作
    Q_UNUSED(image)
    Q_UNUSED(id)
}

std::unique_ptr<QImage> ThumbnailCache::readThumbnail(QStringView id) {
    // 缩略图读取已禁用，直接返回空指针
    Q_UNUSED(id)
    return nullptr;
}

QString ThumbnailCache::thumbnailPath(QStringView id) const {
    // 返回空字符串，因为不需要实际的文件路径
    Q_UNUSED(id)
    return QString();
}

bool ThumbnailCache::exists(QStringView id) const noexcept {
    // 缩略图不存在，因为功能已禁用
    Q_UNUSED(id)
    return false;
}