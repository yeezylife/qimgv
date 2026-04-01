#pragma once

#include <memory>

class Image;

class CacheItem {
public:
    explicit CacheItem(std::shared_ptr<Image> contents);

    std::shared_ptr<Image> getContents() const;

private:
    std::shared_ptr<Image> mContents;
};