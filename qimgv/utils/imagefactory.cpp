#include "imagefactory.h"
#include <memory>
#include "settings.h"

std::shared_ptr<Image> ImageFactory::createImage(const QString& path) {
    // 仅 GUI 线程同步路径调用，此处读设置安全
    return createImage(path, settings->memoryAllocationLimit(),
                       settings->jxlAnimation(), settings->videoPlayback());
}

std::shared_ptr<Image> ImageFactory::createImage(const QString& path, int allocationLimitMB,
                                                 bool jxlAnimation, bool videoPlayback) {
    auto docInfo = std::make_unique<DocumentInfo>(path, jxlAnimation, videoPlayback);
    switch (docInfo->type()) {
    case NONE:
        qDebug() << "ImageFactory: cannot load" << docInfo->filePath();
        return nullptr;
    case ANIMATED:
        return std::make_shared<ImageAnimated>(std::move(docInfo), allocationLimitMB);
    case VIDEO:
        return std::make_shared<Video>(std::move(docInfo));
    default:
        return std::make_shared<ImageStatic>(std::move(docInfo), allocationLimitMB);
    }
}