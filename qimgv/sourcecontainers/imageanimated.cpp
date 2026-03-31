#include "imageanimated.h"

#include <QFile>
#include <utility>
#include <atomic>

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

    // 连接帧更新（GUI线程）
    connect(movie.get(), &QMovie::frameChanged,
            this, &ImageAnimated::onFrameChanged);

    // 初始化第一帧
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
    // ✅ 优先使用缓存（避免 QMovie 调用）
    auto frame = std::atomic_load_explicit(
        &cachedFrame,
        std::memory_order_acquire
    );

    if (frame && !frame->isNull()) {
        outPixmap = QPixmap::fromImage(*frame);
        return;
    }

    // fallback（极少发生）
    if (movie && movie->isValid()) {
        outPixmap = movie->currentPixmap();
        if (!outPixmap.isNull())
            return;
    }

    const QByteArray formatBytes = mDocInfo->format().toLatin1();
    outPixmap = QPixmap(mPath, formatBytes.constData());
}

std::shared_ptr<const QImage> ImageAnimated::getImage() const {
    return std::atomic_load_explicit(
        &cachedFrame,
        std::memory_order_acquire
    );
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

void ImageAnimated::onFrameChanged(int /*frameNumber*/) {
    // ⚠️ 保证在 GUI 线程调用（QMovie 限制）

    QImage img = movie->currentImage();
    if (img.isNull())
        return;

    // ✅ 避免默认构造 + 赋值
    auto newFrame = std::make_shared<QImage>(std::move(img));

    // ✅ lock-free 原子发布
    std::atomic_store_explicit(
        &cachedFrame,
        std::const_pointer_cast<const QImage>(newFrame),
        std::memory_order_release
    );
}