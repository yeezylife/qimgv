#include "cacheitem.h"

CacheItem::CacheItem(std::shared_ptr<Image> contents)
    : mContents(std::move(contents))
{
}

std::shared_ptr<Image> CacheItem::getContents() const {
    return mContents;
}