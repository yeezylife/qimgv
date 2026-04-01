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

    alignas(64) std::atomic<bool> mNeedProcessQueue{false};
    alignas(64) mutable std::shared_mutex mRWLock;

    std::mutex mAccessQueueMutex;
    std::vector<QString> mAccessQueue;
    size_t mQueueThreshold{4};

    QHash<QString, ListIt> items;
    std::list<Node> lruList;

    int mMaxCacheSize{20};

    void moveToFront(ListIt it);
    void evictLRUItems();
    void processAccessQueue();
};