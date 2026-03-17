#include "cache.h"
#include <mutex>
#include <algorithm>

Cache::Cache() : mMaxCacheSize(50) {}

bool Cache::contains(const QString &path) const {
    std::shared_lock locker(mRWLock);
    return items.contains(path);
}

std::shared_ptr<Image> Cache::get(const QString &path) {
    std::shared_lock locker(mRWLock); // 全程使用读锁
    auto it = items.find(path);
    if (it != items.end()) {
        auto item = it.value();
        item->updateAccessTime(); // 原子操作，无需写锁
        return item->getContents();
    }
    return nullptr;
}

bool Cache::insert(const std::shared_ptr<Image> &img) {
    if (!img) return false;

    std::unique_lock locker(mRWLock); // 插入仍需写锁
    const QString &path = img->filePath();

    if (items.contains(path)) return false;

    if (mMaxCacheSize > 0 && items.size() >= mMaxCacheSize) {
        evictLRUItems();
        if (items.size() >= mMaxCacheSize) return false;
    }

    auto newItem = std::make_shared<CacheItem>(img);
    newItem->updateAccessTime();
    items.insert(path, newItem);
    return true;
}

void Cache::evictLRUItems() {
    // 调用者已持有 unique_lock
    if (mMaxCacheSize <= 0 || items.size() <= mMaxCacheSize) return;

    while (items.size() > mMaxCacheSize) {
        QString oldestPath;
        uint64_t oldestTime = std::numeric_limits<uint64_t>::max();
        bool foundAny = false;

        // 扫描寻找：1. 未锁定 2. 时间戳最早的项目
        auto it = items.begin();
        while (it != items.end()) {
            auto item = it.value();
            if (!item->isLocked()) {
                uint64_t t = item->lastAccessTime();
                if (t < oldestTime) {
                    oldestTime = t;
                    oldestPath = it.key();
                    foundAny = true;
                }
            }
            ++it;
        }

        if (foundAny) {
            items.remove(oldestPath);
        } else {
            break; // 所有项目都被锁定，无法继续淘汰
        }
    }
}

void Cache::remove(const QString &path) {
    std::shared_ptr<CacheItem> itemPtr;
    {
        std::unique_lock locker(mRWLock);
        itemPtr = items.take(path);
    }

    if (itemPtr && !itemPtr->tryLock(5000)) {
        qWarning() << "Cache::remove() - Lock timeout for:" << path;
    }
}

void Cache::clear() {
    QList<std::shared_ptr<CacheItem>> itemsToClear;
    {
        std::unique_lock locker(mRWLock);
        auto values = items.values();
        for (auto &v : values) itemsToClear.append(v);
        items.clear();
    }

    for (auto &item : itemsToClear) {
        if (item) item->tryLock(5000);
    }
}

bool Cache::reserve(const QString &path) {
    std::shared_lock locker(mRWLock);
    auto it = items.find(path);
    if (it == items.end()) return false;
    it.value()->lock();
    return true;
}

bool Cache::release(const QString &path) {
    std::shared_lock locker(mRWLock);
    auto it = items.find(path);
    if (it == items.end()) return false;
    it.value()->unlock();
    return true;
}

void Cache::trimTo(const QStringList &pathList) {
    QSet<QString> keepSet(pathList.begin(), pathList.end());
    QList<std::shared_ptr<CacheItem>> toRemove;
    {
        std::unique_lock locker(mRWLock);
        auto it = items.begin();
        while (it != items.end()) {
            if (!keepSet.contains(it.key())) {
                toRemove.append(it.value());
                it = items.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto &i : toRemove) i->tryLock(5000);
}

const QList<QString> Cache::keys() const {
    std::shared_lock locker(mRWLock);
    return items.keys();
}

void Cache::setMaxCacheSize(int maxItems) {
    std::unique_lock locker(mRWLock);
    mMaxCacheSize = maxItems;
    evictLRUItems();
}

int Cache::maxCacheSize() const {
    std::shared_lock locker(mRWLock);
    return mMaxCacheSize;
}

int Cache::currentCacheSize() const {
    std::shared_lock locker(mRWLock);
    return static_cast<int>(items.size());
}