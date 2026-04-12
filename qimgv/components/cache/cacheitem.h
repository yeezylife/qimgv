#pragma once

#include <memory>

class Image;

class CacheItem {
public:
    explicit CacheItem(std::shared_ptr<Image> contents) noexcept;

    std::shared_ptr<Image> getContents() const noexcept;
    void setContents(std::shared_ptr<Image> contents) noexcept;

private:
    // ⚠️ 必须 mutable（因为 atomic_load 需要非 const 指针）
    mutable std::shared_ptr<Image> mContents;
};