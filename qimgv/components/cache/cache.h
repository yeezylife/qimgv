#pragma once

#include <QDebug>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <memory>
#include <list>
#include "sourcecontainers/image.h"
#include "components/cache/cacheitem.h"

class Cache {
public:
    explicit Cache();
    ~Cache() = default;

    bool contains(QString path) const;
    void remove(QString path);
    void clear();

    bool insert(std::shared_ptr<Image> img);
    void trimTo(QStringList list);

    std::shared_ptr<Image> get(QString path);
    bool release(QString path);
    bool reserve(QString path);
    const QList<QString> keys() const;

    // LRU 缓存管理功能
    void setMaxCacheSize(int maxItems);
    int maxCacheSize() const;
    int currentCacheSize() const;

private:
    QHash<QString, std::shared_ptr<CacheItem>> items;   // 缓存项存储
    std::list<QString> lruList;                          // LRU 顺序链表（前端最近使用）
    QHash<QString, std::list<QString>::iterator> lruMap; // 路径到链表迭代器的映射
    int mMaxCacheSize;
    mutable QMutex mMutex;

    // 内部辅助方法
    void updateLRU(const QString& path);
    void evictLRUItems();   // 尝试淘汰最久未使用的未锁定项目
};