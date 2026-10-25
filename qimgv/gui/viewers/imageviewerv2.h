#pragma once

#include <QCanvasPainterWidget>
#include <QCanvasImage>
#include <QImage>
#include <QElapsedTimer>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPropertyAnimation>
#include <QColor>
#include <QHash>
#include <QList>
#include <QPair>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QTimer>
#include <memory>
#include "settings.h"

class ImageAnimated;
QT_BEGIN_NAMESPACE
class QCanvasPainter;
QT_END_NAMESPACE

enum MouseInteractionState {
    MOUSE_NONE,
    MOUSE_DRAG_BEGIN,
    MOUSE_DRAG,
    MOUSE_PAN,
    MOUSE_ZOOM,
    MOUSE_WHEEL_ZOOM
};

enum ViewLockMode {
    LOCK_NONE,
    LOCK_ZOOM,
    LOCK_ALL
};

// Image display widget. All static images and every decoded animation frame are
// drawn by QCanvasPainter (Qt 6.12 Canvas Painter / QRhi). There is deliberately
// no QGraphicsView fallback path.
class ImageViewerV2 : public QCanvasPainterWidget
{
    Q_OBJECT
public:
    explicit ImageViewerV2(QWidget* parent = nullptr);
    ~ImageViewerV2() override;

    ImageFitMode fitMode() const;
    QRect scaledRectR() const;
    float currentScale() const;
    QSize sourceSize() const;
    bool isDisplaying() const;
    bool imageFits() const;
    QWidget* widget();

    bool scaledImageFits() const;
    bool hasAnimation() const;
    QSize scaledSizeR() const;

public slots:
    void setFitMode(ImageFitMode mode);
    void setFitOriginal();
    void setFitWidth();
    void setFitWindow();
    void setFitWindowStretch();
    void zoomIn();
    void zoomOut();
    void zoomInCursor();
    void zoomOutCursor();
    void readSettings();
    void scrollUp();
    void scrollDown();
    void scrollLeft();
    void scrollRight();
    void startAnimation();
    // 仅在动画已暂停且没有等待首帧时恢复播放；供幻灯片启动使用，
    // 避免在加载在途/已经播放时重启计时器。
    void resumeAnimation();
    void stopAnimation();
    void closeImage();
    void setExpandImage(bool mode);
    void show();
    void hide();
    void setLoopPlayback(bool mode);
    void toggleTransparencyGrid();
    void nextFrame();
    void prevFrame();
    bool showAnimationFrame(int frame);
    void onFullscreenModeChanged(bool mode);
    void toggleLockZoom();
    bool lockZoomEnabled() const;
    void toggleLockView();
    bool lockViewEnabled() const;

    void showImage(const QImage& image);
    void showImage(QImage&& image);
    void showAnimation(const std::shared_ptr<ImageAnimated>& animation);
    // 静态图异步缩放结果：requestToken 用于丢弃快速切图/连续变焦后的过期结果
    void setScaledImage(const QImage& newFrame, quint64 requestToken);
    void setScaledImage(QImage&& newFrame, quint64 requestToken);
    void enableDrags();
    void disableDrags();
    void pauseResume();

signals:
    void scalingRequested(QSize size, quint64 requestToken);
    // silent=true：非交互缩放（打开图片自动适配、窗口缩放等），供指示器决定是否闪动
    void scaleChanged(qreal scale, bool silent);
    void sourceSizeChanged(QSize size);
    void imageAreaChanged(QRect rect);
    void draggedOut();
    void playbackFinished();
    void animationPaused(bool paused);
    void frameChanged(int frame);
    void durationChanged(int frames);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool event(QEvent* ev) override;

    // QCanvasPainterWidget
    void initializeResources(QCanvasPainter* painter) override;
    void paint(QCanvasPainter* painter) override;
    void graphicsResourcesInvalidated() override;

private slots:
    void onAnimationTimer();
    void requestScaling();
    void scrollToX(int x);
    void scrollToY(int y);
    void centerOnPixmap();
    void onScrollTimelineFinished();
    void onDPRChanged();

private:
    Q_PROPERTY(int scrollX READ scrollX WRITE setScrollX)
    Q_PROPERTY(int scrollY READ scrollY WRITE setScrollY)

    int scrollX() const;
    void setScrollX(int x);
    int scrollY() const;
    void setScrollY(int y);

    // Full-resolution image (static) or current full-resolution animation frame.
    QImage mSourceImage;
    // Frame that is actually displayed. It can be mSourceImage or a CPU-scaled
    // version produced by the existing scaler / animation scaled-frame cache.
    QImage mDisplayImage;
    bool mDisplayingScaled = false;
    std::shared_ptr<ImageAnimated> movie;
    // 缩放帧缓存：键为帧号，仅在 mScaledFrameSize 匹配时有效。
    // mScaledFrameOrder 维护 LRU（front 最旧、back 最新），与 hash 的键集合保持一致。
    QHash<int, QImage> mScaledFrameCache;
    QList<int> mScaledFrameOrder;
    QSize mScaledFrameSize;
    QImage mCheckerboardImage;

    // --- Canvas Painter GPU resources ---
    // Static images keep their full-resolution texture while a CPU-downscaled
    // texture is displayed, so continuous zooming does not re-upload the full
    // image on every interaction. Animation frames intentionally do not use this
    // cache: only the currently displayed frame owns a texture.
    QCanvasImage mSourceTexture;
    qint64 mSourceTextureKey = -1;
    quint64 mSourceTextureGeneration = 0;
    bool mSourceTextureValid = false;
    QCanvasImage mImageTexture;
    QCanvasImage mCheckerTexture;
    // Old QCanvasImage handles cannot be released outside a painter callback.
    // They are moved here and released from paint() on the next frame.
    QList<QCanvasImage> mPendingTextureReleases;
    qint64 mTextureKey = -1;
    quint64 mTextureGeneration = 0;
    quint64 mTextureDisplaySerial = 0;
    bool mTextureValid = false;
    // 静态图/动画每次显示内容变化都递增，用于丢弃过期异步结果与失效纹理
    quint64 mImageGeneration = 0;
    quint64 mDisplaySerial = 0;
    // 异步缩放请求令牌：只有最近一次请求、且当前图片未变化的结果才可落地
    quint64 mScalingRequestSerial = 0;
    quint64 mActiveScalingRequest = 0;

    // Scene-free view state. Coordinates are logical pixels: the image occupies
    // (0, 0, sourceLogicalSize * scale); mViewCenter is the scene point shown at
    // the viewport center. This replaces QGraphicsView's scene/scrollbar math.
    QPointF mViewCenter;
    float mScale = 1.0f;

    QTimer* animationTimer;
    QTimer* scaleTimer;
    QPropertyAnimation* scrollAnimationX;
    QPropertyAnimation* scrollAnimationY;
    QElapsedTimer lastTouchpadScroll;

    QPoint mouseMoveStartPos;
    QPoint mousePressPos;
    MouseInteractionState mouseInteraction;
    QPair<QPointF, QPoint> zoomAnchor;
    bool scaledImageFitsCache;

    bool transparencyGrid;
    bool expandImage;
    bool keepFitMode;
    bool loopPlayback;
    bool mIsFullscreen;
    // 切回动画时首帧可能已被环形缓冲回收：等首帧解码出来再启动播放计时器，
    // 避免计时器按旧播放头继续请求、与首帧请求互相覆盖
    bool mStartAnimationOnFrameReady = false;
    bool mSilentScaleChange = false;   // 非交互缩放期间置 true，scaleChanged 带上标记
    bool useFixedZoomLevels;
    bool trackpadDetection;
    bool dragsEnabled;
    bool wayland;

    QList<float> zoomLevels;
    float zoomStep;
    float dpr;
    float minScale;
    float maxScale;
    float fitWindowScale;
    float fitWindowStretchScale;
    float expandLimit;
    float lockedScale;
    float lastRequestedScale;
    QPointF savedViewportPos;
    ViewLockMode mViewLock;
    ImageFitMode imageFitMode;
    ImageFitMode imageFitModeDefault;
    ImageFocusPoint focusIn1to1;

    int zoomThreshold;
    int dragThreshold;

    static constexpr int SCROLL_UPDATE_RATE = 7;
    static constexpr int DEFAULT_SCROLL_DISTANCE = 240;
    static constexpr qreal TRACKPAD_SCROLL_MULTIPLIER = 0.7;
    static constexpr qreal WHEEL_SCROLL_MULTIPLIER = 2.0;
    static constexpr int ANIMATION_SPEED = 150;
    static constexpr float FAST_SCALE_THRESHOLD = 1.0f;
    // 后台预解码尚未产出目标帧时的重试间隔
    static constexpr int FRAME_PENDING_RETRY_MS = 10;

    static constexpr int ZOOM_THRESHOLD_FACTOR = 4;

    // 保守的跨平台纹理上限。超出时先降采样再上传，绝不回退 QGraphicsView。
    // 8192 covers 4K/8K sources while staying within the guaranteed texture
    // limits of older desktop GPUs; larger sources are downsampled for upload.
    static constexpr int MAX_TEXTURE_DIMENSION = 8192;
    static constexpr qsizetype MAX_TEXTURE_BYTES =
        static_cast<qsizetype>(256) * 1024 * 1024;

    void initSettings();
    void setFitModeImpl(ImageFitMode mode);
    bool imageFitsInternal() const;
    float currentScaleInternal() const;

    void initializeTimers();
    void setupConnections();

    void adjustZoom(bool zoomIn, bool atCursor);
    void zoomAnchored(float newScale);
    // 去抖请求缩放：交互变焦连续触发时只在停顿后算一次（scaleTimer, 80ms）
    void scheduleScaling();
    void doZoom(float newScale, bool center = true);
    void setZoomAnchor(QPoint viewportPos);
    quint64 invalidateScalingRequests();

    // 动画缩小播放的缩放帧缓存（按目标尺寸整体失效，容量按内存预算做有界 LRU）
    QImage scaledFrameFor(int frameNumber, const QImage &frame, const QSize &target);
    void clearScaledFrameCache();

    void fitNormal();
    void fitWidth();
    void fitWindow();
    void fitWindowStretch();
    void fitFree(float scale);
    void applyFitMode();
    // 打开图片/动画后应用初始适配，期间 scaleChanged 标记为 silent
    void applyInitialFit();
    void updateFitWindowScale();
    void updateFitWindowStretchScale();
    void updateMinScale();

    void scroll(int dx, int dy, bool animated);
    void scrollSmooth(int dx, int dy);
    void scrollPrecise(int dx, int dy);
    void stopPosAnimation();

    void mousePan(QMouseEvent* event);
    void mouseMoveZoom(QMouseEvent* event);
    void handleWheelZoom(QWheelEvent* event);
    void handleTrackpadScroll(QWheelEvent* event);
    void handleMouseWheelScroll(QWheelEvent* event);

    void adjustViewport();
    QPointF snappedCenter(QPointF center) const;
    QPointF clampViewportCenter(QPointF center) const;
    void saveViewportPos();
    void applySavedViewportPos();
    void lockZoom();

    // --- image/geometry helpers replacing QGraphicsScene/QGraphicsItem ---
    QSizeF sourceLogicalSize() const;
    QSizeF displayedLogicalSize() const;
    QRectF imageRectScene() const;
    QRectF imageRectViewport() const;
    QRectF viewportRectF() const;
    QPointF mapToScene(QPoint viewportPos) const;
    QPointF mapToScene(QPointF viewportPos) const;
    QRectF mapToScene(QRect viewportRect) const;
    QPointF mapFromScene(QPointF scenePos) const;
    QRectF mapFromScene(QRectF sceneRect) const;
    void centerOn(QPointF scenePos);
    void centerOn(qreal x, qreal y);

    void setSourceImage(const QImage& image);
    void setAnimationFrame(const QImage& frame);
    void setAnimationScaledFrame(const QImage& frame);
    void swapToOriginalImage();
    void invalidateImageTexture();
    void invalidateSourceTexture();
    void ensureImageTexture(QCanvasPainter* painter);
    void ensureSourceTexture(QCanvasPainter* painter);
    void releasePendingTextures(QCanvasPainter* painter);
    void paintCheckerboard(QCanvasPainter* painter, const QRectF& area);
    QImage prepareTextureImage(const QImage& image) const;
    static bool imageHasTransparency(const QImage& image);
    void updateBackgroundColor();
    void queueTextureRelease(const QCanvasImage& texture);

    void reset();
};
