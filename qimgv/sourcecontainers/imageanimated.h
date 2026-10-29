#pragma once

#include "image.h"
#include <QImageReader>
#include <QList>
#include <QMutex>
#include <atomic>
#include <memory>

class ImageAnimated final : public Image {
public:
    // 任意线程可调用：解码限额已由调用方在 GUI 线程快照
    ImageAnimated(std::unique_ptr<DocumentInfo> _info, int allocationLimitMB);
    ~ImageAnimated() override;

    using Image::save;

    void getPixmap(QPixmap& outPixmap, qreal devicePixelRatio) const override;
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
    QPixmap currentPixmap(qreal devicePixelRatio = 1.0) const;

    // 帧缓存（有界环形窗口）+ reader 的共享状态（定义在 .cpp）：
    // 后台解码任务与 GUI 线程共享。公开仅为让 .cpp 里的解码任务能命名该类型，外部不应使用
    struct DecodeContext;

    // ⭐ 后台预解码开关。GUI 线程永不调用 read()，帧未就绪时 jumpToFrame() 返回
    // false（不阻塞）。三者分工：
    //   startDecoding(): 恢复/确保解码任务运行
    //   pauseDecoding(): 暂停推进（保留 reader 与已解码窗口，恢复无需重解码前缀）
    //   stopDecoding():  让解码任务退出（切走/关闭时调用，及时让出线程池）；
    //                    任务真正停止后回收帧窗口，不阻塞 GUI
    void startDecoding();
    void pauseDecoding();
    void stopDecoding();
    // 后台是否仍在解码：false 表示缓存已到末尾，此时缺帧就是真的不可用
    bool isDecoding() const;

public slots:
    bool save() override;
    bool save(QString destPath) override;

private:
    void loadWith(int allocationLimitMB);
    void loadMovie(int allocationLimitMB);
    std::shared_ptr<const QImage> frameAt(int index) const;

    QSize mSize;
    std::atomic<int> mFrameCount{0};
    int mCurrentFrame = -1;

    std::shared_ptr<DecodeContext> mCtx;

    // 当前帧 QPixmap 缓存：同一帧 + 同一 dpr 只转换一次（dpr 缓存直接读 QPixmap 自身）
    mutable QPixmap mCurrentPixmap;
    mutable bool mCurrentPixmapValid = false;

    // ✅ 使用普通 shared_ptr + atomic free functions（跨线程给 getImage() 消费者）
    std::shared_ptr<const QImage> cachedFrame;
};
