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

    // -----------------------------
    // 1️⃣ 高频原子标志独占 cache line
    alignas(64) std::atomic<bool> mNeedProcessQueue{false};

    // -----------------------------
    // 2️⃣ 高频锁独占 cache line
    alignas(64) mutable std::shared_mutex mRWLock;

    // -----------------------------
    // 3️⃣ 队列相关
    mutable std::mutex mAccessQueueMutex;
    std::vector<QString> mAccessQueue;
    size_t mQueueThreshold{64}; // 默认阈值

    // -----------------------------
    // 4️⃣ 核心 LRU 数据
    QHash<QString, ListIt> items;
    std::list<Node> lruList;

    // -----------------------------
    // 5️⃣ 配置参数
    int mMaxCacheSize{20};

    // -----------------------------
    // 辅助函数
    void moveToFront(ListIt it);
    void evictLRUItems();
    void processAccessQueue();
};