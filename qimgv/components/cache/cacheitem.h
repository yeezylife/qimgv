#pragma once

#include <QSemaphore>
#include <memory>                      // for std::shared_ptr
#include "sourcecontainers/image.h"

class CacheItem {
public:
    CacheItem() = default;
    explicit CacheItem(std::shared_ptr<Image> contents);

    ~CacheItem() = default;

    [[nodiscard]] std::shared_ptr<Image> getContents() const;

    void lock();
    void unlock();
    bool tryLock(int timeoutMs);       // timeout in milliseconds

    [[nodiscard]] int lockStatus() const;
    [[nodiscard]] bool isLocked() const;

private:
    std::shared_ptr<Image> contents;
    QSemaphore sem{1};
};