#include "imagefactory.h"
#include <memory>

std::shared_ptr<Image> ImageFactory::createImage(const QString& path) {
    auto docInfo = std::make_unique<DocumentInfo>(path);
    switch (docInfo->type()) {
    case NONE:
        qDebug() << "ImageFactory: cannot load" << docInfo->filePath();
        return nullptr;
    case ANIMATED:
        return std::make_shared<ImageAnimated>(std::move(docInfo));
    case VIDEO:
        return std::make_shared<Video>(std::move(docInfo));
    default:
        return std::make_shared<ImageStatic>(std::move(docInfo));
    }
}