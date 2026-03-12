#include "cacheitem.h"

CacheItem::CacheItem() = default;

CacheItem::CacheItem(std::shared_ptr<Image> contents)
    : contents(std::move(contents)) {
    // 可以是空的，也可以放一些初始化逻辑
}

std::shared_ptr<Image> CacheItem::getContents() {
    return contents;
}

void CacheItem::lock() {
    sem.acquire(1);
}

void CacheItem::unlock() {
    sem.release(1);
}

bool CacheItem::tryLock(int timeout) {
    // QSemaphore::tryAcquire(int n, int timeout)
    // timeout 单位为毫秒
    // 返回 true 表示成功获取信号量，false 表示超时
    return sem.tryAcquire(1, timeout);
}

int CacheItem::lockStatus() {
    return sem.available();
}

bool CacheItem::isLocked() const {
    return sem.available() == 0;
}
