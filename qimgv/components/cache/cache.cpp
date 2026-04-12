#include "cache.h"

Cache::Cache(size_t maxSize)
    : mMaxCacheSize(maxSize)
{
}

bool Cache::contains(const QString &path) const {
    std::shared_lock lock(mRWLock);
    return items.contains(path);
}

std::shared_ptr<Image> Cache::get(const QString &path) {
    std::shared_ptr<Image> result;

    {
        std::shared_lock lock(mRWLock);

        auto it = items.find(path);
        if (it == items.end()) return nullptr;

        // fast path：只记录访问
        {
            std::lock_guard qlock(mAccessQueueMutex);
            mAccessQueue.push_back(path);

            if (mAccessQueue.size() > mQueueThreshold) {
                mNeedProcessQueue.store(true, std::memory_order_release);
            }
        }

        result = it.value()->item->getContents();
    }

    // 慢路径：批量处理 LRU 顺序
    if (mNeedProcessQueue.exchange(false, std::memory_order_acq_rel)) {
        std::unique_lock lock(mRWLock);
        processAccessQueue();
    }

    return result;
}

bool Cache::insert(const std::shared_ptr<Image> &img) {
    if (!img) return false;

    std::unique_lock lock(mRWLock);

    const QString &path = img->filePath();

    auto it = items.find(path);
    if (it != items.end()) {
        // ✅ 原子原地更新，严格保持原有返回语义
        it.value()->item->setContents(img);
        moveToFront(it.value());
        return false;
    }

    lruList.push_front({path, std::make_shared<CacheItem>(img)});
    items.insert(path, lruList.begin());

    if (items.size() > mMaxCacheSize) {
        evictLRUItems();
    }

    return true;
}

void Cache::remove(const QString &path) {
    std::unique_lock lock(mRWLock);

    auto it = items.find(path);
    if (it == items.end()) return;

    lruList.erase(it.value());
    items.erase(it);
}

void Cache::clear() {
    std::unique_lock lock(mRWLock);
    lruList.clear();
    items.clear();
}

void Cache::moveToFront(ListIt it) {
    if (it == lruList.begin()) return;
    lruList.splice(lruList.begin(), lruList, it);
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

void Cache::evictLRUItems() {
    // 可选：保持 LRU 顺序新鲜
    processAccessQueue();

    while (items.size() > mMaxCacheSize) {
        auto lastIt = std::prev(lruList.end());

        items.remove(lastIt->key);
        lruList.pop_back(); // O(1)
    }
}