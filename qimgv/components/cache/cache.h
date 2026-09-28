#pragma once

#include <QHash>
#include <QString>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include "sourcecontainers/image.h"

class Cache {
public:
    // maxSize 为个数上限（兼容旧语义）；字节上限默认 2048MB，避免高像素 OOM
    explicit Cache(size_t maxSize = 20, size_t maxBytes = 2048ULL * 1024ULL * 1024ULL);

    bool contains(const QString &path) const;
    std::shared_ptr<Image> get(const QString &path);
    // 单锁取出并删除，用于 modify 路径合并 3 次加锁为 1 次
    std::shared_ptr<Image> take(const QString &path);
    bool insert(const std::shared_ptr<Image> &img);

    void remove(const QString &path);
    void clear();

    void setMaxBytes(size_t bytes) { mMaxBytes = bytes; }

private:
    struct Node {
        QString key;
        std::shared_ptr<Image> item;
        size_t bytes = 0;
    };

    using ListIt = std::list<Node>::iterator;

private:
    void moveToFront(ListIt it);
    void evictLRUItems();
    static size_t estimateBytes(const std::shared_ptr<Image> &img) noexcept;

private:
    size_t mMaxCacheSize;
    size_t mMaxBytes;
    size_t mCurrentBytes = 0;

    std::list<Node> lruList;                 // front = MRU, back = LRU
    QHash<QString, ListIt> items;

    mutable std::shared_mutex mRWLock;
};
