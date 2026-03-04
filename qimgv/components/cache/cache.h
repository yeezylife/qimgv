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

    // 添加 LRU 缓存管理功能
    void setMaxCacheSize(int maxItems);
    int maxCacheSize() const;
    int currentCacheSize() const;

private:
    // 使用 QHash 替代 QMap，查找速度更快 (O(1) vs O(log n))
    // 使用 shared_ptr 自动管理内存，无需手动 delete，且支持容器复制语义
    QHash<QString, std::shared_ptr<CacheItem>> items;
    
    // LRU 链表，用于跟踪访问顺序
    std::list<QString> lruList;
    QHash<QString, std::list<QString>::iterator> lruMap;
    
    // 缓存大小限制
    int mMaxCacheSize;
    
    // 增加互斥锁保证线程安全
    mutable QMutex mMutex;
    
    // 内部辅助方法
    void updateLRU(const QString& path);
    void evictLRUItems();
};
