#pragma once

#include <QHash>
#include <QStringList>
#include <shared_mutex> // C++17 读写锁
#include <memory>
#include <vector>
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
    const QList<QString> keys() const;

    void setMaxCacheSize(int maxItems);
    int maxCacheSize() const;
    int currentCacheSize() const;

private:
    QHash<QString, std::shared_ptr<CacheItem>> items;
    int mMaxCacheSize;
    mutable std::shared_mutex mRWLock; // 现代标准库读写锁

    void evictLRUItems(); // 现在基于时间戳进行扫描淘汰
};