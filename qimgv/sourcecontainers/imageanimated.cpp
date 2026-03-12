#include "imageanimated.h"
#include <QDebug>
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
    // 保持使用 std::shared_ptr 逻辑，但建议用 make_shared (如有性能需求)
    movie = std::make_shared<QMovie>();
    movie->setFileName(mPath);
    movie->setFormat(mDocInfo->format().toLatin1());
    movie->jumpToFrame(0);
    mSize = movie->frameRect().size();
    mFrameCount = movie->frameCount();
}

int ImageAnimated::frameCount() {
    return mFrameCount;
}

bool ImageAnimated::save(QString destPath) {
    QFile file(mPath);
    if (file.exists()) {
        if (!file.copy(destPath)) {
            qDebug() << "Unable to save file.";
            return false;
        } else {
            if (destPath == this->filePath()) {
                mDocInfo->refresh();
            }
            return true;
        }
    } else {
        qDebug() << "Unable to save file. Perhaps the source file was deleted?";
        return false;
    }
}

bool ImageAnimated::save() {
    return false;
}

std::unique_ptr<QPixmap> ImageAnimated::getPixmap() const {
    const QByteArray formatBytes = mDocInfo->format().toLatin1();
    return std::make_unique<QPixmap>(mPath, formatBytes.constData());
}

std::shared_ptr<const QImage> ImageAnimated::getImage() const {
    const QByteArray formatBytes = mDocInfo->format().toLatin1();
    // 显式指明类型以匹配 shared_ptr<const QImage>
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