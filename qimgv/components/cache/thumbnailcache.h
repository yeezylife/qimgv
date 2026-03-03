#pragma once

#include <QObject>
#include <QDir>
#include <QMutex>
#include <QImage>
#include <QStringView>
#include <memory>
#include "settings.h"
#include "sourcecontainers/thumbnail.h"

class ThumbnailCache : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ThumbnailCache)

public:
    explicit ThumbnailCache(QObject *parent = nullptr);
    ~ThumbnailCache() override = default;

    void saveThumbnail(const QImage &image, QStringView id);
    std::unique_ptr<QImage> readThumbnail(QStringView id);
    QString thumbnailPath(QStringView id) const;
    bool exists(QStringView id) const noexcept;

private:
    mutable QMutex mutex;
    QString cacheDirPath;
};