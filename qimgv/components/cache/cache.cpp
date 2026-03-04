#include "cache.h"

Cache::Cache() {
    mMaxCacheSize = 30; // 默认缓存最多30个项目
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
        QMutexLocker locker(&mMutex);
        // 先从缓存结构中移除，同时清理 LRU 信息
        if (lruMap.contains(path)) {
            lruList.erase(lruMap[path]);
            lruMap.remove(path);
        }
        itemPtr = items.take(path);
    }

    // 等待项目解锁（如果正在使用），然后自动释放
    if (itemPtr) {
        itemPtr->lock();   // 阻塞直到解锁
        // shared_ptr 离开作用域自动删除
    }
}

void Cache::clear() {
    QList<std::shared_ptr<CacheItem>> itemsToClear;

    {
        QMutexLocker locker(&mMutex);
        // 清空所有内部数据结构
        lruList.clear();
        lruMap.clear();

        auto keys = items.keys();
        for (const auto &key : keys) {
            itemsToClear.append(items.take(key));
        }
    }

    // 安全释放所有项目
    for (auto &item : itemsToClear) {
        if (item) {
            item->lock();   // 等待解锁
            // shared_ptr 自动释放
        }
    }
}

std::shared_ptr<Image> Cache::get(QString path) {
    QMutexLocker locker(&mMutex);
    auto it = items.find(path);
    if (it != items.end()) {
        updateLRU(path);   // 标记为最近使用
        return it.value()->getContents();
    }
    return nullptr;
}

bool Cache::reserve(QString path) {
    std::shared_ptr<CacheItem> item;
    {
        QMutexLocker locker(&mMutex);
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
        QMutexLocker locker(&mMutex);
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
        QMutexLocker locker(&mMutex);
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

void Cache::setMaxCacheSize(int maxItems) {
    QMutexLocker locker(&mMutex);
    mMaxCacheSize = maxItems;
    if (mMaxCacheSize > 0 && items.size() > mMaxCacheSize) {
        evictLRUItems();
    }
}

int Cache::maxCacheSize() const {
    QMutexLocker locker(&mMutex);
    return mMaxCacheSize;
}

int Cache::currentCacheSize() const {
    QMutexLocker locker(&mMutex);
    return items.size();
}

void Cache::updateLRU(const QString& path) {
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
    // 如果最大缓存大小 <= 0（无限制），直接返回
    if (mMaxCacheSize <= 0) {
        return;
    }

    // 从链表尾部（最久未使用）向前查找未锁定的项目进行淘汰
    auto it = lruList.end();
    while (items.size() > mMaxCacheSize && it != lruList.begin()) {
        --it;  // 从最后一个元素开始
        const QString &path = *it;
        auto itemIt = items.find(path);
        if (itemIt == items.end()) {
            // 理论上不应出现，但若出现则清理脏数据
            it = lruList.erase(it);
            lruMap.remove(path);
            continue;
        }

        // 检查项目是否锁定
        if (!itemIt.value()->isLocked()) {
            // 未锁定，可以淘汰
            // 从 LRU 结构中移除
            it = lruList.erase(it);
            lruMap.remove(path);
            // 从缓存中移除（shared_ptr 离开作用域自动释放）
            items.erase(itemIt);
            // 注意：erase 后迭代器可能失效，需要重新定位到末尾
            it = lruList.end();
        } else {
            // 锁定项目不能淘汰，继续向前查找
        }
    }
}