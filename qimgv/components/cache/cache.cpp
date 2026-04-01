#include "cache.h"
#include <algorithm>

Cache::Cache()
    : mMaxCacheSize(20)
{
    mAccessQueue.reserve(128);
    mQueueThreshold = std::max<size_t>(4, mMaxCacheSize / 4);
}

bool Cache::contains(const QString &path) const {
    std::shared_lock locker(mRWLock);
    return items.contains(path);
}

std::shared_ptr<Image> Cache::get(const QString &path) {
    std::shared_ptr<Image> result;

    {
        std::shared_lock locker(mRWLock);
        auto it = items.find(path);
        if (it == items.end()) return nullptr;

        {
            std::lock_guard qlock(mAccessQueueMutex);
            mAccessQueue.push_back(path);

            if (mAccessQueue.size() > mQueueThreshold) {
                mNeedProcessQueue.store(true, std::memory_order_release);
            }
        }

        result = it.value()->item->getContents();
    }

    // 🔥 批处理更新 + 顺带淘汰
    if (mNeedProcessQueue.exchange(false, std::memory_order_acq_rel)) {
        std::unique_lock locker(mRWLock);
        processAccessQueue();

        if (items.size() > mMaxCacheSize) {
            evictLRUItems();
        }
    }

    return result;
}

bool Cache::insert(const std::shared_ptr<Image> &img) {
    if (!img) return false;

    std::unique_lock locker(mRWLock);

    processAccessQueue();

    const QString &path = img->filePath();

    if (items.contains(path)) return false;

    if (mMaxCacheSize > 0 && items.size() >= mMaxCacheSize) {
        evictLRUItems();
    }

    lruList.push_front({path, std::make_shared<CacheItem>(img)});
    items.insert(path, lruList.begin());

    return true;
}

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
    processAccessQueue();

    if (items.size() <= mMaxCacheSize) return;

    auto it = lruList.end();

    while (items.size() > mMaxCacheSize && it != lruList.begin()) {
        --it;

        if (it->item->isLocked()) {
            continue;
        }

        items.remove(it->key);
        it = lruList.erase(it);
    }
}

void Cache::remove(const QString &path) {
    std::unique_lock locker(mRWLock);

    processAccessQueue();

    auto it = items.find(path);
    if (it == items.end()) return;

    lruList.erase(it.value());
    items.erase(it);
}

void Cache::clear() {
    std::unique_lock locker(mRWLock);

    lruList.clear();
    items.clear();
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
    mQueueThreshold = std::max<size_t>(4, mMaxCacheSize / 4);
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