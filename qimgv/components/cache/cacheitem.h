#pragma once

#include <QSemaphore>
#include "sourcecontainers/image.h"

class CacheItem {
public:
    CacheItem();
    CacheItem(std::shared_ptr<Image> _contents);
    ~CacheItem() = default;

    std::shared_ptr<Image> getContents();

    void lock();
    void unlock();
    bool tryLock(int timeout);  // 新增：带超时的锁定方法

    int lockStatus();
    bool isLocked() const;
    
private:
    std::shared_ptr<Image> contents;
    QSemaphore sem{1};  // 改为值成员，避免手动管理内存
};
