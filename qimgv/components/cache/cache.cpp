#include "cache.h"

Cache::Cache() {
}

bool Cache::contains(QString path) const {
    QMutexLocker locker(&mMutex);
    return items.contains(path);
}

bool Cache::insert(std::shared_ptr<Image> img) {
    if (!img) {
        return false;
    }
    
    QMutexLocker locker(&mMutex);
    const QString &path = img->filePath();
    
    // 如果已存在，则不插入
    if (items.contains(path)) {
        return false;
    }
    
    // 使用 make_shared 创建对象
    items.insert(path, std::make_shared<CacheItem>(img));
    return true;
}

void Cache::remove(QString path) {
    std::shared_ptr<CacheItem> itemPtr;
    
    {
        QMutexLocker locker(&mMutex);
        itemPtr = items.take(path); // 从 map 中移除并获取所有权
    }
    
    // 锁定并在作用域结束时自动释放
    // 注意：不要在持有 mMutex 时等待 item->lock()，否则会阻塞其他线程访问缓存
    if (itemPtr) {
        itemPtr->lock(); // 等待资源释放 (如果正在使用中)
        // shared_ptr 离开作用域自动删除对象，无需手动 delete
    }
}

void Cache::clear() {
    // 先将所有 item 从 map 中取出，清空 map
    QList<std::shared_ptr<CacheItem>> itemsToClear;
    {
        QMutexLocker locker(&mMutex);
        // 遍历 keys 并 take 出来
        auto keys = items.keys();
        for (const auto &key : keys) {
            itemsToClear.append(items.take(key));
        }
    }
    
    // 在不持有全局锁的情况下安全释放资源
    for (auto &item : itemsToClear) {
        if (item) {
            item->lock();
            // shared_ptr 自动释放
        }
    }
}

std::shared_ptr<Image> Cache::get(QString path) {
    QMutexLocker locker(&mMutex);
    if (items.contains(path)) {
        return items.value(path)->getContents();
    }
    return nullptr;
}

bool Cache::reserve(QString path) {
    QMutexLocker locker(&mMutex);
    if (items.contains(path)) {
        items.value(path)->lock();
        return true;
    }
    return false;
}

bool Cache::release(QString path) {
    QMutexLocker locker(&mMutex);
    if (items.contains(path)) {
        items.value(path)->unlock();
        return true;
    }
    return false;
}

// 移除不在列表中的所有项
void Cache::trimTo(QStringList pathList) {
    // 转为 QSet 进行 O(1) 查找，大幅提升性能
    QSet<QString> keepSet(pathList.begin(), pathList.end());
    
    QList<std::shared_ptr<CacheItem>> itemsToRemove;
    
    {
        QMutexLocker locker(&mMutex);
        auto keys = items.keys();
        for (const auto &key : keys) {
            if (!keepSet.contains(key)) {
                itemsToRemove.append(items.take(key));
            }
        }
    }
    
    // 安全释放
    for (auto &item : itemsToRemove) {
        if (item) {
            item->lock();
        }
    }
}

const QList<QString> Cache::keys() const {
    QMutexLocker locker(&mMutex);
    return items.keys();
}
