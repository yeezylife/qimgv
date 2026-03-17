#include "cacheitem.h"

CacheItem::CacheItem(std::shared_ptr<Image> contents)
    : contents(std::move(contents)), lockedFlag(false)
{
}

std::shared_ptr<Image> CacheItem::getContents() const
{
    return contents;
}

void CacheItem::lock()
{
    sem.acquire(); // 阻塞直到获得信号量
    lockedFlag.store(true, std::memory_order_release);
}

void CacheItem::unlock()
{
    lockedFlag.store(false, std::memory_order_release);
    sem.release(); // 释放信号量
}

bool CacheItem::tryLock(int timeoutMs)
{
    // 使用 C++20 的超时获取
    auto timeout = std::chrono::milliseconds(timeoutMs);
    if (sem.try_acquire_for(timeout)) {
        lockedFlag.store(true, std::memory_order_release);
        return true;
    }
    return false;
}

bool CacheItem::isLocked() const
{
    // 原子性读取，开销极低
    return lockedFlag.load(std::memory_order_acquire);
}