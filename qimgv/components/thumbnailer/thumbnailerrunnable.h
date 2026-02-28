#pragma once

#include <QRunnable>
#include <QProcess>
#include <QThread>
#include <QCryptographicHash>
#include <ctime>
#include "sourcecontainers/thumbnail.h"
#include "components/cache/thumbnailcache.h"
#include "utils/imagefactory.h"
#include "utils/imagelib.h"
#include "settings.h"
#include <memory>
#include <QImageWriter>

class ThumbnailerRunnable : public QObject, public QRunnable {
    Q_OBJECT
public:
    // 优化：使用 const 引用传递字符串，避免拷贝
    ThumbnailerRunnable(ThumbnailCache* _cache, const QString &_path, int _size, bool _crop, bool _force);
    ~ThumbnailerRunnable();
    void run() override;
    
    // 优化：静态生成函数同样使用 const 引用
    static std::shared_ptr<Thumbnail> generate(ThumbnailCache *cache, const QString &path, int size, bool crop, bool force);
    
private:
    static QString generateIdString(const QString &path, int size, bool crop);
    // 以下两个耗时函数实际上不再会被调用，但保留声明以维持结构完整性
    static std::pair<QImage*, QSize> createThumbnail(const QString &path, const char* format, int size, bool crop);
    static std::pair<QImage*, QSize> createVideoThumbnail(const QString &path, int size, bool crop);
    
    QString path;
    int size;
    bool crop, force;
    ThumbnailCache* cache = nullptr;

signals:
    void taskStart(QString, int);
    void taskEnd(std::shared_ptr<Thumbnail>, QString);
};
