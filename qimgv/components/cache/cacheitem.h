#pragma once

#include <memory>
#include <atomic>
#include "sourcecontainers/image.h"

class CacheItem {
public:
    CacheItem() = default;
    explicit CacheItem(std::shared_ptr<Image> contents);
    ~CacheItem() = default;

    [[nodiscard]] std::shared_ptr<Image> getContents() const;

    void lock();
    void unlock();
    bool isLocked() const;

private:
    std::shared_ptr<Image> contents;

    // 🔥 无锁引用计数
    std::atomic<int> refCount{0};
};