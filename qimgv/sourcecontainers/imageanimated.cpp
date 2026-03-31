#include "imageanimated.h"
#include <QFile>
#include <utility>

ImageAnimated::ImageAnimated(QString _path)
    : Image(std::move(_path))
{
    mSize = QSize(0, 0);
    ImageAnimated::load();
}

ImageAnimated::ImageAnimated(std::unique_ptr<DocumentInfo> _info)
    : Image(std::move(_info))
{
    mSize = QSize(0, 0);
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

    // ✅ 连接帧变化信号（确保 QMovie 只在 GUI 线程操作）
    connect(movie.get(), &QMovie::frameChanged, this, &ImageAnimated::onFrameChanged);

    // ✅ 初始化第一帧缓存
    onFrameChanged(0);
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
    return cachedFrame.load(std::memory_order_acquire);
}

std::shared_ptr<QMovie> ImageAnimated::getMovie() {
    if (!movie)
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

void ImageAnimated::onFrameChanged(int frameNumber) {
    // ✅ 在 GUI 线程中无锁更新缓存（使用 C++20 atomic<shared_ptr>）
    QImage img = movie->currentImage();
    if (!img.isNull()) {
        cachedFrame.store(std::make_shared<QImage>(std::move(img)), std::memory_order_release);
    }
}
