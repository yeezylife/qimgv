#include "imageanimated.h"

#include <QFile>
#include <atomic>
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
    // 仅 GUI 线程同步路径调用，此处读设置安全；异步解码走 loadWith(快照)
    loadWith(settings->memoryAllocationLimit());
}

ImageAnimated::ImageAnimated(std::unique_ptr<DocumentInfo> _info, int allocationLimitMB)
    : Image(std::move(_info))
{
    mSize = QSize(0, 0);
    loadWith(allocationLimitMB);
}

void ImageAnimated::loadWith(int allocationLimitMB) {
    if (isLoaded())
        return;

    loadMovie(allocationLimitMB);
    mLoaded = true;
}

void ImageAnimated::loadMovie(int allocationLimitMB) {
    if (mReader)
        return;

    auto reader = std::make_unique<QImageReader>(mPath);

    reader->setAllocationLimit(allocationLimitMB);

    if (!reader->canRead()) {
        mSize = QSize(0, 0);
        mFrameCount = 0;
        return;
    }

    mReader = std::move(reader);
    mFrameCount = qMax(0, mReader->imageCount());

    // 解码第一帧并发布（getImage() 消费者无需播放即可取到当前帧）
    if (!jumpToFrame(0)) {
        mSize = QSize(0, 0);
        mFrameCount = 0;
        return;
    }

    mSize = mFrames.isEmpty() ? QSize(0, 0) : mFrames.at(0)->size();
}

int ImageAnimated::frameCount() const {
    return mFrameCount;
}

int ImageAnimated::currentFrameNumber() const {
    return mCurrentFrame;
}

int ImageAnimated::nextFrameDelay() const {
    if (mCurrentFrame < 0 || mCurrentFrame >= mDelays.size())
        return 0;
    return mDelays.at(mCurrentFrame);
}

bool ImageAnimated::isValid() const {
    // 有效 = 至少解出第 0 帧；imageCount() 返回 0/-1（帧数未知）时也能显示，
    // 不再要求扫描帧数 > 0（旧实现此处会把可读但帧数未知的 WebP/AVIF 判死为空白）
    return mReader && !mFrames.isEmpty();
}

bool ImageAnimated::jumpToFrame(int frameNumber) {
    if (frameNumber < 0 || (mFrameCount > 0 && frameNumber >= mFrameCount))
        return false;

    if (frameNumber == mCurrentFrame && frameNumber < mFrames.size())
        return true;

    const auto frame = decodeFrame(frameNumber);
    if (!frame)
        return false;

    mCurrentFrame = frameNumber;
    mCurrentPixmapValid = false;

    // 发布给 getImage() 消费者（打印/剪贴板等），仅一次 shared_ptr 原子交换
    std::atomic_store_explicit(&cachedFrame, frame, std::memory_order_release);
    return true;
}

bool ImageAnimated::jumpToNextFrame() {
    return jumpToFrame(mCurrentFrame + 1);
}

std::shared_ptr<const QImage> ImageAnimated::decodeFrame(int index) {
    if (index < 0)
        return nullptr;
    if (index < mFrames.size())
        return mFrames.at(index);

    // 顺序解码直到目标帧：动画格式连续调用 read() 即返回下一帧（GIF 即如此）
    bool reachedEnd = false;
    while (mFrames.size() <= index) {
        QImage img = mReader->read();
        if (img.isNull()) {
            reachedEnd = true;
            break;
        }

        mDelays.append(qMax(10, mReader->nextImageDelay()));
        mFrames.append(std::make_shared<const QImage>(std::move(img)));
    }

    // 实际帧数少于 imageCount() 的扫描值时修正
    if (reachedEnd && mFrames.size() < mFrameCount)
        mFrameCount = static_cast<int>(mFrames.size());

    return (index < mFrames.size()) ? mFrames.at(index) : nullptr;
}

QPixmap ImageAnimated::currentPixmap() const {
    if (mCurrentFrame < 0 || mCurrentFrame >= mFrames.size())
        return QPixmap();

    // 同一帧只做一次 QImage→QPixmap，循环播放时后续直接共享
    if (!mCurrentPixmapValid) {
        mCurrentPixmap = QPixmap::fromImage(*mFrames.at(mCurrentFrame));
        mCurrentPixmapValid = true;
    }
    return mCurrentPixmap;
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
    // ✅ 优先使用当前帧的 QPixmap 缓存（同一帧只转换一次）
    outPixmap = currentPixmap();
    if (!outPixmap.isNull())
        return;

    // fallback（极少发生）
    const QByteArray formatBytes = mDocInfo->format().toLatin1();
    outPixmap = QPixmap(mPath, formatBytes.constData());
}

std::shared_ptr<const QImage> ImageAnimated::getImage() const {
    return std::atomic_load_explicit(&cachedFrame, std::memory_order_acquire);
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
