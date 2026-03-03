#pragma once

#include <QDebug>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <memory>
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

private:
    // 使用 QHash 替代 QMap，查找速度更快 (O(1) vs O(log n))
    // 使用 shared_ptr 自动管理内存，无需手动 delete，且支持容器复制语义
    QHash<QString, std::shared_ptr<CacheItem>> items;
    
    // 增加互斥锁保证线程安全
    mutable QMutex mMutex;
};
