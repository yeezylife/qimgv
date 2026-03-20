#include "cacheitem.h"

CacheItem::CacheItem(std::shared_ptr<Image> contents)
    : contents(std::move(contents))
{
}

std::shared_ptr<Image> CacheItem::getContents() const
{
    return contents;
}

void CacheItem::lock()
{
    sem.acquire(); // 阻塞直到获得信号量
    lockedFlag.store(true, std::memory_order_relaxed);
}

void CacheItem::unlock()
{
    // ✅ 关键修复：先释放信号量，再更新状态
    // 避免短暂“未锁定但实际还未释放”的窗口
    sem.release();
    lockedFlag.store(false, std::memory_order_relaxed);
}

bool CacheItem::tryLock(int timeoutMs)
{
    auto timeout = std::chrono::milliseconds(timeoutMs);
    if (sem.try_acquire_for(timeout)) {
        lockedFlag.store(true, std::memory_order_relaxed);
        return true;
    }
    return false;
}

bool CacheItem::isLocked() const
{
    // 仅用于快速判断（允许短暂不一致）
    return lockedFlag.load(std::memory_order_relaxed);
}