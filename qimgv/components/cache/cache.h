#pragma once

#include <QHash>
#include <QStringList>
#include <shared_mutex>
#include <memory>
#include <list>
#include <atomic>
#include <vector>
#include <mutex>

#include "sourcecontainers/image.h"
#include "components/cache/cacheitem.h"

class Cache {
public:
    explicit Cache();
    ~Cache() = default;

    bool contains(const QString &path) const;
    void remove(const QString &path);
    void clear();
    bool insert(const std::shared_ptr<Image> &img);
    std::shared_ptr<Image> get(const QString &path);
    bool release(const QString &path);
    bool reserve(const QString &path);
    QList<QString> keys() const;

    void setMaxCacheSize(int maxItems);
    int maxCacheSize() const;
    int currentCacheSize() const;

private:
    struct Node {
        QString key;
        std::shared_ptr<CacheItem> item;
    };

    using ListIt = std::list<Node>::iterator;

    std::list<Node> lruList;
    QHash<QString, ListIt> items;

    int mMaxCacheSize;

    mutable std::shared_mutex mRWLock;

    // 🚀 延迟访问队列
    mutable std::mutex mAccessQueueMutex;
    std::vector<QString> mAccessQueue;

    // 🚀 批处理阈值（🔥 新增）
    size_t mQueueThreshold;

    // 🚀 原子标志
    alignas(64) std::atomic<bool> mNeedProcessQueue{false};

    void moveToFront(ListIt it);
    void evictLRUItems();

    void processAccessQueue();
};