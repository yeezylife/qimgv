#include "video.h"

Video::Video(QString _path) : Image(std::move(_path)) {
    load();
}

Video::Video(std::unique_ptr<DocumentInfo> _info) : Image(std::move(_info)) {
    load();
}

void Video::load() {
    if (isLoaded())
        return;

    // Video metadata resolution is not available here without an external decoder.
    // Keep the loader lightweight and avoid expensive external calls during container creation.
    srcWidth = 0;
    srcHeight = 0;
    mLoaded = true;
}

bool Video::save(QString /*destPath*/) {
    return false;
}

bool Video::save() {
    return false;
}

void Video::getPixmap(QPixmap& outPixmap) const {
    static const QPixmap emptyPixmap;
    outPixmap = emptyPixmap;
}

std::shared_ptr<const QImage> Video::getImage() const {
    static const std::shared_ptr<const QImage> emptyImage = std::make_shared<const QImage>();
    return emptyImage;
}

int Video::height() const {
    return static_cast<int>(srcHeight);
}

int Video::width() const {
    return static_cast<int>(srcWidth);
}

QSize Video::size() const {
    return QSize(static_cast<int>(srcWidth), static_cast<int>(srcHeight));
}
