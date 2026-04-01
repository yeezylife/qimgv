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
    refCount.fetch_add(1, std::memory_order_relaxed);
}

void CacheItem::unlock()
{
    refCount.fetch_sub(1, std::memory_order_relaxed);
}

bool CacheItem::isLocked() const
{
    return refCount.load(std::memory_order_relaxed) > 0;
}