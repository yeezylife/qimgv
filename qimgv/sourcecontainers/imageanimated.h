#pragma once

#include "image.h"
#include <QImageReader>
#include <QList>
#include <memory>

class ImageAnimated final : public Image {
public:
    explicit ImageAnimated(QString _path);
    // 仅 GUI 线程同步路径调用：解码限额内部读取设置
    explicit ImageAnimated(std::unique_ptr<DocumentInfo> _info);
    // 任意线程可调用：解码限额已由调用方在 GUI 线程快照
    ImageAnimated(std::unique_ptr<DocumentInfo> _info, int allocationLimitMB);
    ~ImageAnimated() override = default;

    using Image::save;

    void getPixmap(QPixmap& outPixmap) const override;
    std::shared_ptr<const QImage> getImage() const override;
    int height() const override;
    int width() const override;
    QSize size() const override;

    int frameCount() const;
    int currentFrameNumber() const;
    int nextFrameDelay() const;
    bool isValid() const;
    bool jumpToFrame(int frameNumber);
    bool jumpToNextFrame();
    QPixmap currentPixmap() const;

public slots:
    bool save() override;
    bool save(QString destPath) override;

private:
    void load() override;
    void loadWith(int allocationLimitMB);
    void loadMovie(int allocationLimitMB);
    std::shared_ptr<const QImage> decodeFrame(int index);

    QSize mSize;
    int mFrameCount = 0;
    int mCurrentFrame = -1;
    std::unique_ptr<QImageReader> mReader;

    // 帧缓存（等价 QMovie::CacheAll：按需顺序解码并全部缓存，循环播放时直接复用）
    QList<std::shared_ptr<const QImage>> mFrames;
    QList<int> mDelays;

    // 当前帧 QPixmap 缓存：QMovie 每帧都丢弃重建，这里同一帧只转换一次
    mutable QPixmap mCurrentPixmap;
    mutable bool mCurrentPixmapValid = false;

    // ✅ 使用普通 shared_ptr + atomic free functions（跨线程给 getImage() 消费者）
    std::shared_ptr<const QImage> cachedFrame;
};
