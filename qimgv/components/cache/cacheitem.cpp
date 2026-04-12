#include "cacheitem.h"

CacheItem::CacheItem(std::shared_ptr<Image> contents) noexcept
    : mContents(std::move(contents))
{
}

std::shared_ptr<Image> CacheItem::getContents() const noexcept {
    // acquire 保证读取到完整对象
    return std::atomic_load_explicit(&mContents, std::memory_order_acquire);
}

void CacheItem::setContents(std::shared_ptr<Image> contents) noexcept {
    // release 保证写入对其他线程可见
    std::atomic_store_explicit(&mContents, std::move(contents), std::memory_order_release);
}