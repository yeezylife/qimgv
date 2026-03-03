#include "thumbnailcache.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QImageWriter>

ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent)
    , cacheDirPath(settings->thumbnailCacheDir())
{
    QDir dir(cacheDirPath);
    if(!dir.exists())
        dir.mkpath(".");
}

QString ThumbnailCache::thumbnailPath(QStringView id) const {
    return cacheDirPath + QLatin1Char('/') + id.toString() + u".png";
}

bool ThumbnailCache::exists(QStringView id) const noexcept {
    return QFile::exists(thumbnailPath(id));
}

void ThumbnailCache::saveThumbnail(const QImage &image, QStringView id) {
    if(image.isNull())
        return;
    
    const QString filePath = thumbnailPath(id);
    
    // Qt 6: QSaveFile 原子写入，防止文件损坏
    QSaveFile saveFile(filePath);
    if(!saveFile.open(QIODevice::WriteOnly))
        return;
    
    QImageWriter writer(&saveFile, "PNG");
    writer.setCompression(15);
    writer.write(image);
    saveFile.commit();
}

std::unique_ptr<QImage> ThumbnailCache::readThumbnail(QStringView id) {
    const QString filePath = thumbnailPath(id);
    
    if(!QFile::exists(filePath))
        return nullptr;
    
    const QMutexLocker locker(&mutex);
    
    auto image = std::make_unique<QImage>();
    if(!image->load(filePath))
        return nullptr;
    
    return image;
}