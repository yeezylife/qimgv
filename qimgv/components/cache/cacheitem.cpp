#include "cacheitem.h"

CacheItem::CacheItem() = default;

CacheItem::CacheItem(std::shared_ptr<Image> contents)
    : contents(std::move(contents))
{
    // 构造函数主体可以留空，或放其他初始化逻辑
}

std::shared_ptr<Image> CacheItem::getContents() const
{
    return contents;
}

void CacheItem::lock()
{
    sem.acquire(1);
}

void CacheItem::unlock()
{
    sem.release(1);
}

bool CacheItem::tryLock(int timeoutMs)
{
    return sem.tryAcquire(1, timeoutMs);
}

int CacheItem::lockStatus() const
{
    return sem.available();
}

bool CacheItem::isLocked() const
{
    return sem.available() == 0;
}