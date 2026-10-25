#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QElapsedTimer>
#include <QWheelEvent>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QColor>
#include <QHash>
#include <QList>
#include <QTimer>
#include <memory>
#include "settings.h"

class ImageAnimated;

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

class ImageViewerV2 : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ImageViewerV2(QWidget* parent = nullptr);
    ~ImageViewerV2();

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

    void showImage(const QPixmap& pixmap);
    void showImage(QPixmap&& pixmap);
    void showAnimation(const std::shared_ptr<ImageAnimated>& animation);
    void setScaledPixmap(const QPixmap& newFrame, quint64 viewerToken);
    void setScaledPixmap(QPixmap&& newFrame, quint64 viewerToken);
    void enableDrags();
    void disableDrags();
    void pauseResume();

signals:
    void scalingRequested(QSize size, quint64 viewerToken);
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
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

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

    QGraphicsScene* scene;
    std::unique_ptr<QPixmap> pixmap;
    QPixmap pixmapScaled;
    std::shared_ptr<ImageAnimated> movie;
    // 缩放帧缓存：键为帧号，仅在 mScaledFrameSize 匹配时有效。
    // mScaledFrameOrder 维护 LRU（front 最旧、back 最新），与 hash 的键集合保持一致。
    QHash<int, QPixmap> mScaledFrameCache;
    QList<int> mScaledFrameOrder;
    QSize mScaledFrameSize;
    // 视图令牌：换图/重置/切动画/DPR 变化时递增；在途缩放结果必须带回同一令牌才允许显示
    quint64 mViewerToken = 0;
    QGraphicsPixmapItem pixmapItem;
    QGraphicsPixmapItem pixmapItemScaled;
    QPixmap checkboard;

    QTimer* animationTimer;
    QTimer* scaleTimer;
    QScrollBar* horizontalScroll;
    QScrollBar* verticalScroll;
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
    bool scrollBarWorkaround;
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

    static constexpr int SCENE_SIZE = 200000;
    static constexpr int CENTER_OFFSET = 10000;
    static constexpr int ZOOM_THRESHOLD_FACTOR = 4;

    void initSettings();
    void setFitModeImpl(ImageFitMode mode);
    bool imageFitsInternal() const;
    float currentScaleInternal() const;

    void initializeScene();
    void initializeTimers();
    void initializeScrollBars();
    void setupConnections();

    void adjustZoom(bool zoomIn, bool atCursor);
    void zoomAnchored(float newScale);
    // 去抖请求缩放：交互变焦连续触发时只在停顿后算一次（scaleTimer, 80ms）
    void scheduleScaling();
    void doZoom(float newScale, bool center = true);
    void setZoomAnchor(QPoint viewportPos);

    // 动画缩小播放的缩放帧缓存（按目标尺寸整体失效，容量按内存预算做有界 LRU）
    QPixmap scaledFrameFor(int frameNumber, const QImage &frame, const QSize &target);
    void clearScaledFrameCache();
    // 使所有在途 CPU 缩放结果失效（换图、重置、切动画、DPR 变化、不再需要缩放时）
    void invalidateViewerToken() noexcept { ++mViewerToken; }

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

    void updatePixmap(const QPixmap& newPixmap);
    void updatePixmap(QPixmap&& newPixmap);
    void swapToOriginalPixmap();
    // ⭐ showImage 双重载公共内核：reset→装配→fit→请求缩放一次完成
    void showImageImpl(QPixmap pixmap);

    void reset();
    QPointF sceneRoundPos(QPointF scenePoint) const;
    QRectF sceneRoundRect(QRectF sceneRect) const;
};