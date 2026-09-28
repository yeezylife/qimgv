#include "cache.h"
#include <algorithm>

Cache::Cache(size_t maxSize, size_t maxBytes)
    : mMaxCacheSize(maxSize)
    , mMaxBytes(maxBytes)
{
}

size_t Cache::estimateBytes(const std::shared_ptr<Image> &img) noexcept {
    // ⭐ 解码后 QImage 真实像素占用为准，避免按个数淘汰时 20 张高像素即 OOM
    if(img) {
        if(auto qimg = img->getImage()) {
            const qsizetype bytes = qimg->sizeInBytes();
            if(bytes > 0)
                return static_cast<size_t>(bytes);
        }
        // 动图/未就绪兜底：至少按文件大小计，避免 0 字节条目永不淘汰
        const qint64 fileBytes = img->fileSize();
        if(fileBytes > 0)
            return static_cast<size_t>(fileBytes);
    }
    return 1024ULL * 1024ULL;
}

bool Cache::contains(const QString &path) const {
    std::shared_lock lock(mRWLock);
    return items.contains(path);
}

std::shared_ptr<Image> Cache::get(const QString &path) {
    // ⭐ 读多写少：先共享锁探查，已在 MRU 头则直接返回，省掉每次命中的写锁
    {
        std::shared_lock lock(mRWLock);
        auto it = items.find(path);
        if(it == items.end())
            return nullptr;
        if(it.value() == lruList.begin())
            return it.value()->item;
    }
    std::unique_lock lock(mRWLock);

    auto it = items.find(path);
    if (it == items.end()) return nullptr;

    // 命中即提升到 MRU 头，保证淘汰时 LRU 顺序始终新鲜
    moveToFront(it.value());
    return it.value()->item;
}

std::shared_ptr<Image> Cache::take(const QString &path) {
    // ⭐ 单锁 get+erase，供 onFileModified 合并 contains+remove+get 三次加锁
    std::unique_lock lock(mRWLock);
    auto it = items.find(path);
    if(it == items.end())
        return nullptr;
    auto img = it.value()->item;
    mCurrentBytes -= it.value()->bytes;
    lruList.erase(it.value());
    items.erase(it);
    return img;
}

bool Cache::insert(const std::shared_ptr<Image> &img) {
    if (!img) return false;

    std::unique_lock lock(mRWLock);

    const QString &path = img->filePath();
    const size_t bytes = estimateBytes(img);

    auto it = items.find(path);
    if (it != items.end()) {
        // 原地更新，保持原有返回语义（false = 已存在，非新增）
        mCurrentBytes -= it.value()->bytes;
        it.value()->item = img;
        it.value()->bytes = bytes;
        mCurrentBytes += bytes;
        moveToFront(it.value());
        if (mCurrentBytes > mMaxBytes || items.size() > mMaxCacheSize)
            evictLRUItems();
        return false;
    }

    lruList.push_front({path, img, bytes});
    items.insert(path, lruList.begin());
    mCurrentBytes += bytes;

    if (mCurrentBytes > mMaxBytes || items.size() > mMaxCacheSize) {
        evictLRUItems();
    }

    return true;
}

void Cache::remove(const QString &path) {
    std::unique_lock lock(mRWLock);

    auto it = items.find(path);
    if (it == items.end()) return;

    mCurrentBytes -= it.value()->bytes;
    lruList.erase(it.value());
    items.erase(it);
}

void Cache::clear() {
    std::unique_lock lock(mRWLock);
    lruList.clear();
    items.clear();
    mCurrentBytes = 0;
}

void Cache::moveToFront(ListIt it) {
    if (it == lruList.begin()) return;
    lruList.splice(lruList.begin(), lruList, it);
}

void Cache::evictLRUItems() {
    while (!lruList.empty() &&
           (items.size() > mMaxCacheSize || mCurrentBytes > mMaxBytes)) {
        auto lastIt = std::prev(lruList.end());

        mCurrentBytes -= lastIt->bytes;
        items.remove(lastIt->key);
        lruList.pop_back(); // O(1)
    }
}
