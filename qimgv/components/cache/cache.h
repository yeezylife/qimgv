#pragma once

#include <QHash>
#include <QString>
#include <list>
#include <memory>
#include <shared_mutex>
#include <vector>
#include <atomic>
#include <mutex>

#include "sourcecontainers/image.h"
#include "components/cache/cacheitem.h"

class Cache {
public:
    explicit Cache(size_t maxSize = 20);

    bool contains(const QString &path) const;
    std::shared_ptr<Image> get(const QString &path);
    bool insert(const std::shared_ptr<Image> &img);

    void remove(const QString &path);
    void clear();

private:
    struct Node {
        QString key;
        std::shared_ptr<CacheItem> item;
    };

    using ListIt = std::list<Node>::iterator;

private:
    void moveToFront(ListIt it);
    void evictLRUItems();
    void processAccessQueue();

private:
    size_t mMaxCacheSize;

    std::list<Node> lruList;                 // front = MRU, back = LRU
    QHash<QString, ListIt> items;

    mutable std::shared_mutex mRWLock;

    // fast path queue（减少写锁争用）
    std::vector<QString> mAccessQueue;
    std::mutex mAccessQueueMutex;
    std::atomic<bool> mNeedProcessQueue{false};

    static constexpr size_t mQueueThreshold = 5;
};