#include "imageanimated.h"
#include <QFile>
#include <utility>

// 修复 performance-unnecessary-value-param: 使用 std::move
ImageAnimated::ImageAnimated(QString _path)
    : Image(std::move(_path))
{
    mSize = QSize(0, 0);
    // 修复 VirtualCall: 使用类名限定符显式调用，杜绝虚函数分发风险
    ImageAnimated::load();
}

ImageAnimated::ImageAnimated(std::unique_ptr<DocumentInfo> _info)
    : Image(std::move(_info))
{
    mSize = QSize(0, 0);
    // 修复 VirtualCall
    ImageAnimated::load();
}

void ImageAnimated::load() {
    if (isLoaded())
        return;
    loadMovie();
    mLoaded = true;
}

void ImageAnimated::loadMovie() {
    if (movie && movie->fileName() == mPath && movie->isValid()) {
        return;
    }

    movie = std::make_shared<QMovie>(mPath);
    movie->setCacheMode(QMovie::CacheAll);
    if (!movie->isValid()) {
        mSize = QSize(0, 0);
        mFrameCount = 0;
        return;
    }

    movie->jumpToFrame(0);
    mSize = movie->frameRect().size();
    mFrameCount = movie->frameCount();
}

int ImageAnimated::frameCount() {
    return mFrameCount;
}

bool ImageAnimated::save(QString destPath) {
    QFile file(mPath);
    if (!file.exists()) {
        return false;
    }

    if (!file.copy(destPath)) {
        return false;
    }

    if (destPath == this->filePath()) {
        mDocInfo->refresh();
    }
    return true;
}

bool ImageAnimated::save() {
    return false;
}

void ImageAnimated::getPixmap(QPixmap& outPixmap) const {
    if (movie && movie->isValid()) {
        outPixmap = movie->currentPixmap();
        if (!outPixmap.isNull())
            return;
    }

    const QByteArray formatBytes = mDocInfo->format().toLatin1();
    outPixmap = QPixmap(mPath, formatBytes.constData());
}

std::shared_ptr<const QImage> ImageAnimated::getImage() const {
    if (movie && movie->isValid()) {
        const QImage img = movie->currentImage();
        if (!img.isNull()) {
            return std::make_shared<const QImage>(img);
        }
    }

    const QByteArray formatBytes = mDocInfo->format().toLatin1();
    return std::make_shared<const QImage>(mPath, formatBytes.constData());
}

std::shared_ptr<QMovie> ImageAnimated::getMovie() {
    if (movie == nullptr)
        loadMovie();
    return movie;
}

int ImageAnimated::height() const {
    return mSize.height();
}

int ImageAnimated::width() const {
    return mSize.width();
}

QSize ImageAnimated::size() const {
    return mSize;
}