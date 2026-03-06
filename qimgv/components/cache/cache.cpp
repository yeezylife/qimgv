#include "cache.h"

Cache::Cache() {
    mMaxCacheSize = 50; // 默认缓存最多50个项目
}

bool Cache::contains(QString path) const {
    QReadLocker locker(&mRWLock);  // 读锁
    return items.contains(path);
}

bool Cache::insert(std::shared_ptr<Image> img) {
    if (!img) {
        return false;
    }

    QWriteLocker locker(&mRWLock);  // 写锁
    const QString &path = img->filePath();

    // 如果已存在，则不插入
    if (items.contains(path)) {
        return false;
    }

    // 检查是否需要淘汰 LRU 项目
    if (mMaxCacheSize > 0 && items.size() >= mMaxCacheSize) {
        evictLRUItems();  // 尝试腾出空间
        // 如果淘汰后仍然满员（例如所有项目都锁定），则插入失败
        if (items.size() >= mMaxCacheSize) {
            return false;
        }
    }

    // 插入新项目
    items.insert(path, std::make_shared<CacheItem>(img));
    updateLRU(path);   // 标记为最近使用

    return true;
}

void Cache::remove(QString path) {
    std::shared_ptr<CacheItem> itemPtr;

    {
        QWriteLocker locker(&mRWLock);  // 写锁
        // 先从缓存结构中移除，同时清理 LRU 信息
        if (lruMap.contains(path)) {
            lruList.erase(lruMap[path]);
            lruMap.remove(path);
        }
        itemPtr = items.take(path);
    }

    // 等待项目解锁（如果正在使用），使用超时避免死锁
    if (itemPtr) {
        // 使用 5 秒超时，避免永久阻塞
        if (!itemPtr->tryLock(5000)) {
            qWarning() << "Cache::remove() - Failed to lock item within timeout:" << path;
            // 即使超时，shared_ptr 也会在作用域结束时尝试释放
        }
        // shared_ptr 离开作用域自动删除
    }
}

void Cache::clear() {
    QList<std::shared_ptr<CacheItem>> itemsToClear;

    {
        QWriteLocker locker(&mRWLock);  // 写锁
        // 清空所有内部数据结构
        lruList.clear();
        lruMap.clear();

        auto keys = items.keys();
        for (const auto &key : keys) {
            itemsToClear.append(items.take(key));
        }
    }

    // 安全释放所有项目（使用超时）
    for (auto &item : itemsToClear) {
        if (item) {
            if (!item->tryLock(5000)) {
                qWarning() << "Cache::clear() - Failed to lock item within timeout";
            }
            // shared_ptr 自动释放
        }
    }
}

std::shared_ptr<Image> Cache::get(QString path) {
    std::shared_ptr<CacheItem> item;
    
    // 第一步：使用读锁获取 CacheItem
    {
        QReadLocker locker(&mRWLock);
        auto it = items.find(path);
        if (it != items.end()) {
            item = it.value();  // 获取 shared_ptr 副本
        }
    }
    
    // 第二步：如果找到，使用写锁更新 LRU（分离锁竞争）
    if (item) {
        QWriteLocker locker(&mRWLock);
        // 再次检查项目是否仍在缓存中（可能在释放读锁后被删除）
        if (items.contains(path)) {
            updateLRU(path);
        }
        return item->getContents();
    }
    
    return nullptr;
}

bool Cache::reserve(QString path) {
    std::shared_ptr<CacheItem> item;
    {
        QReadLocker locker(&mRWLock);  // 读锁
        auto it = items.find(path);
        if (it == items.end()) {
            return false;
        }
        item = it.value();
    }
    // 此时已释放缓存锁，只锁定 CacheItem 内部资源
    item->lock();
    return true;
}

bool Cache::release(QString path) {
    std::shared_ptr<CacheItem> item;
    {
        QReadLocker locker(&mRWLock);  // 读锁
        auto it = items.find(path);
        if (it == items.end()) {
            return false;
        }
        item = it.value();
    }
    item->unlock();
    return true;
}

void Cache::trimTo(QStringList pathList) {
    QSet<QString> keepSet(pathList.begin(), pathList.end());
    QList<std::shared_ptr<CacheItem>> itemsToRemove;

    {
        QWriteLocker locker(&mRWLock);  // 写锁
        auto keys = items.keys();
        for (const auto &key : keys) {
            if (!keepSet.contains(key)) {
                // 从 LRU 结构中移除
                if (lruMap.contains(key)) {
                    lruList.erase(lruMap[key]);
                    lruMap.remove(key);
                }
                // 从缓存中取出
                itemsToRemove.append(items.take(key));
            }
        }
    }

    // 安全释放（使用超时）
    for (auto &item : itemsToRemove) {
        if (item) {
            if (!item->tryLock(5000)) {
                qWarning() << "Cache::trimTo() - Failed to lock item within timeout";
            }
        }
    }
}

const QList<QString> Cache::keys() const {
    QReadLocker locker(&mRWLock);  // 读锁
    return items.keys();
}

void Cache::setMaxCacheSize(int maxItems) {
    QWriteLocker locker(&mRWLock);  // 写锁
    mMaxCacheSize = maxItems;
    if (mMaxCacheSize > 0 && items.size() > mMaxCacheSize) {
        evictLRUItems();
    }
}

int Cache::maxCacheSize() const {
    QReadLocker locker(&mRWLock);  // 读锁
    return mMaxCacheSize;
}

int Cache::currentCacheSize() const {
    QReadLocker locker(&mRWLock);  // 读锁
    return items.size();
}

void Cache::updateLRU(const QString& path) {
    // 调用者必须持有写锁
    
    // 如果已存在，用 splice 移到链表前端（更高效）
    auto it = lruMap.find(path);
    if (it != lruMap.end()) {
        lruList.splice(lruList.begin(), lruList, it.value());
    } else {
        // 新节点插入到前端
        lruList.push_front(path);
        lruMap[path] = lruList.begin();
    }
}

void Cache::evictLRUItems() {
    // 调用者必须持有写锁
    
    // 如果最大缓存大小 <= 0（无限制），直接返回
    if (mMaxCacheSize <= 0) {
        return;
    }

    // 修复：从链表尾部（最久未使用）逐个检查并淘汰
    while (items.size() > mMaxCacheSize && !lruList.empty()) {
        const QString &path = lruList.back();  // 获取最久未使用的路径
        auto itemIt = items.find(path);
        
        if (itemIt == items.end()) {
            // 理论上不应出现，但若出现则清理脏数据
            lruList.pop_back();
            lruMap.remove(path);
            continue;
        }

        // 检查项目是否锁定
        if (!itemIt.value()->isLocked()) {
            // 未锁定，可以淘汰
            lruList.pop_back();
            lruMap.remove(path);
            items.erase(itemIt);  // 从缓存中移除（shared_ptr 自动释放）
        } else {
            // 所有剩余项目都锁定，无法继续淘汰
            break;
        }
    }
}
