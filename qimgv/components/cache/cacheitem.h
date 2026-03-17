#pragma once

#include <semaphore>
#include <memory>
#include <atomic>
#include <chrono>
#include <cstdint>
#include "sourcecontainers/image.h"

class CacheItem {
public:
    // 删除默认构造函数，强制提供 contents
    explicit CacheItem(std::shared_ptr<Image> contents);
    ~CacheItem() = default;

    [[nodiscard]] std::shared_ptr<Image> getContents() const;

    void lock();
    void unlock();
    bool tryLock(int timeoutMs);

    [[nodiscard]] bool isLocked() const;

    void updateAccessTime();
    [[nodiscard]] uint64_t lastAccessTime() const;

private:
    std::shared_ptr<Image> contents;
    std::binary_semaphore sem{1};
    std::atomic<bool> lockedFlag{false};
    std::atomic<uint64_t> mAccessTime{0};

    static std::atomic<uint64_t> sGlobalCounter;
};