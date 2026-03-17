#include "cacheitem.h"

std::atomic<uint64_t> CacheItem::sGlobalCounter{0};

CacheItem::CacheItem(std::shared_ptr<Image> contents)
    : contents(std::move(contents)), 
      lockedFlag(false), 
      mAccessTime(0)
{
    updateAccessTime(); // 构造时设置初始时间戳
}

std::shared_ptr<Image> CacheItem::getContents() const
{
    return contents;
}

void CacheItem::lock()
{
    sem.acquire();
    lockedFlag.store(true, std::memory_order_release);
}

void CacheItem::unlock()
{
    lockedFlag.store(false, std::memory_order_release);
    sem.release();
}

bool CacheItem::tryLock(int timeoutMs)
{
    auto timeout = std::chrono::milliseconds(timeoutMs);
    if (sem.try_acquire_for(timeout)) {
        lockedFlag.store(true, std::memory_order_release);
        return true;
    }
    return false;
}

bool CacheItem::isLocked() const
{
    return lockedFlag.load(std::memory_order_acquire);
}

void CacheItem::updateAccessTime()
{
    uint64_t timestamp = sGlobalCounter.fetch_add(1, std::memory_order_relaxed) + 1;
    mAccessTime.store(timestamp, std::memory_order_release);
}

uint64_t CacheItem::lastAccessTime() const
{
    return mAccessTime.load(std::memory_order_acquire);
}