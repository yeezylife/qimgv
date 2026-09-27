#pragma once

#include "utils/imagelib.h"
#include "sourcecontainers/documentinfo.h"
#include "sourcecontainers/image.h"
#include "sourcecontainers/imageanimated.h"
#include "sourcecontainers/imagestatic.h"
#include "sourcecontainers/video.h"

class ImageFactory {
public:
    // 仅 GUI 线程调用：内部读取设置
    static std::shared_ptr<Image> createImage(const QString& path);
    // 任意线程可调用：解码限额与格式开关已由调用方在 GUI 线程快照
    static std::shared_ptr<Image> createImage(const QString& path, int allocationLimitMB,
                                              bool jxlAnimation, bool videoPlayback);
};