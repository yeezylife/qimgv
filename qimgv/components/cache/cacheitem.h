#pragma once

#include <semaphore>   // C++20 标准信号量
#include <memory>      // std::shared_ptr
#include <atomic>      // std::atomic
#include <chrono>      // std::chrono
#include "sourcecontainers/image.h"

class CacheItem {
public:
    CacheItem() = default;
    explicit CacheItem(std::shared_ptr<Image> contents);
    ~CacheItem() = default;

    // 获取缓存内容
    [[nodiscard]] std::shared_ptr<Image> getContents() const;

    // 锁定与解锁
    void lock();
    void unlock();
    
    // 尝试锁定（带超时）
    bool tryLock(int timeoutMs);

    // 状态检查
    [[nodiscard]] bool isLocked() const;

private:
    std::shared_ptr<Image> contents;
    
    // 初始化为 1，表示默认可用（未锁定）
    std::binary_semaphore sem{1}; 
    
    // 快速状态标记（仅用于优化判断，不保证强一致）
    std::atomic<bool> lockedFlag{false};
};