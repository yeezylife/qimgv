#include "cache.h"
#include <algorithm>

Cache::Cache() : mMaxCacheSize(20) {
    // 🚀 关键优化：避免频繁 realloc
    mAccessQueue.reserve(128);
}

bool Cache::contains(const QString &path) const {
    std::shared_lock locker(mRWLock);
    return items.contains(path);
}

// 🚀 低锁 get（核心优化）
std::shared_ptr<Image> Cache::get(const QString &path) {
    std::shared_ptr<Image> result;

    {
        std::shared_lock locker(mRWLock);
        auto it = items.find(path);
        if (it == items.end()) return nullptr;

        {
            std::lock_guard qlock(mAccessQueueMutex);
            mAccessQueue.push_back(path);

            if (mAccessQueue.size() > 64) {
                mNeedProcessQueue.store(true, std::memory_order_release);
            }
        }

        result = it.value()->item->getContents();
    }

    // 🚀 锁外批处理
    if (mNeedProcessQueue.exchange(false, std::memory_order_acq_rel)) {
        std::unique_lock locker(mRWLock);
        processAccessQueue();
    }

    return result;
}

bool Cache::insert(const std::shared_ptr<Image> &img) {
    if (!img) return false;

    std::unique_lock locker(mRWLock);

    processAccessQueue(); // 🚀 保证 LRU 顺序最新

    const QString &path = img->filePath();

    if (items.contains(path)) return false;

    if (mMaxCacheSize > 0 && items.size() >= mMaxCacheSize) {
        evictLRUItems();
    }

    lruList.push_front({path, std::make_shared<CacheItem>(img)});
    items.insert(path, lruList.begin());

    return true;
}

// 🚀 批量处理访问
void Cache::processAccessQueue() {
    std::vector<QString> localQueue;

    {
        std::lock_guard qlock(mAccessQueueMutex);
        if (mAccessQueue.empty()) return;
        localQueue.swap(mAccessQueue);
    }

    for (const auto &key : localQueue) {
        auto it = items.find(key);
        if (it != items.end()) {
            moveToFront(it.value());
        }
    }
}

void Cache::moveToFront(ListIt it) {
    if (it == lruList.begin()) return;
    lruList.splice(lruList.begin(), lruList, it);
}

void Cache::evictLRUItems() {
    processAccessQueue(); // 确保顺序最新

    while (items.size() > mMaxCacheSize && !lruList.empty()) {
        auto lastIt = std::prev(lruList.end());

        if (lastIt->item->isLocked()) {
            // 🚀 改进：跳过，而不是 break
            lruList.splice(lruList.begin(), lruList, lastIt);
            continue;
        }

        items.remove(lastIt->key);
        lruList.erase(lastIt);
    }
}

void Cache::remove(const QString &path) {
    std::shared_ptr<CacheItem> itemPtr;

    {
        std::unique_lock locker(mRWLock);

        processAccessQueue();

        auto it = items.find(path);
        if (it == items.end()) return;

        auto listIt = it.value();
        itemPtr = listIt->item;

        lruList.erase(listIt);
        items.erase(it);
    }

    if (itemPtr && !itemPtr->tryLock(5000)) {
        qWarning() << "Cache::remove() - Lock timeout for:" << path;
    }
}

void Cache::clear() {
    QList<std::shared_ptr<CacheItem>> itemsToClear;

    {
        std::unique_lock locker(mRWLock);

        for (auto &node : lruList) {
            itemsToClear.append(node.item);
        }

        lruList.clear();
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
    it.value()->item->lock();
    return true;
}

bool Cache::release(const QString &path) {
    std::shared_lock locker(mRWLock);
    auto it = items.find(path);
    if (it == items.end()) return false;
    it.value()->item->unlock();
    return true;
}

QList<QString> Cache::keys() const {
    std::shared_lock locker(mRWLock);

    QList<QString> result;
    result.reserve(static_cast<int>(lruList.size()));

    std::transform(lruList.begin(), lruList.end(), std::back_inserter(result),
                   [](const Node &node) { return node.key; });

    return result;
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