#pragma once

#include <QThreadPool>
#include <memory> // 引入智能指针头文件
#include "components/thumbnailer/thumbnailerrunnable.h"
#include "components/cache/thumbnailcache.h"
#include "settings.h"
#include "sourcecontainers/thumbnail.h"

class Thumbnailer : public QObject
{
    Q_OBJECT
public:
    explicit Thumbnailer();
    ~Thumbnailer();
    
    // 优化：参数使用 const 引用传递，减少拷贝开销
    static std::shared_ptr<Thumbnail> getThumbnail(const QString &filePath, int size);
    
    void clearTasks();
    void waitForDone();

public slots:
    // 优化：参数使用 const 引用传递
    void getThumbnailAsync(const QString &path, int size, bool crop, bool force);

private:
    // 优化：使用智能指针管理 ThumbnailCache，防止内存泄漏
    std::unique_ptr<ThumbnailCache> cache;
    QThreadPool *pool;
    
    void startThumbnailerThread(const QString &filePath, int size, bool crop, bool force);
    QMultiMap<QString, int> runningTasks;

private slots:
    void onTaskStart(const QString &filePath, int size);
    void onTaskEnd(std::shared_ptr<Thumbnail> thumbnail, const QString &filePath);

signals:
    void thumbnailReady(std::shared_ptr<Thumbnail> thumbnail, QString filePath);
};
