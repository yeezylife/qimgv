#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QElapsedTimer>
#include <QWheelEvent>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QMovie>
#include <QColor>
#include <QTimer>
#include <memory>
#include "settings.h"

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
    ScalingFilter scalingFilter() const;
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
    void stopAnimation();
    void closeImage();
    void setExpandImage(bool mode);
    void show();
    void hide();
    void setFilterNearest();
    void setFilterBilinear();
    void setScalingFilter(ScalingFilter filter);
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
    void showAnimation(const std::shared_ptr<QMovie>& animation);
    void setScaledPixmap(const QPixmap& newFrame);
    void setScaledPixmap(QPixmap&& newFrame);
    void enableDrags();
    void disableDrags();
    void pauseResume();

signals:
    void scalingRequested(QSize size, ScalingFilter filter);
    void scaleChanged(qreal scale);
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
    std::shared_ptr<QMovie> movie;
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

    bool transparencyGrid;
    bool expandImage;
    bool smoothAnimatedImages;
    bool smoothUpscaling;
    bool forceFastScale;
    bool keepFitMode;
    bool loopPlayback;
    bool mIsFullscreen;
    bool scrollBarWorkaround;
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
    ScalingFilter mScalingFilter;

    int zoomThreshold;
    int dragThreshold;

    static constexpr int SCROLL_UPDATE_RATE = 7;
    static constexpr int DEFAULT_SCROLL_DISTANCE = 240;
    static constexpr qreal TRACKPAD_SCROLL_MULTIPLIER = 0.7;
    static constexpr qreal WHEEL_SCROLL_MULTIPLIER = 2.0;
    static constexpr int ANIMATION_SPEED = 150;
    static constexpr float FAST_SCALE_THRESHOLD = 1.0f;
    static constexpr int LARGE_VIEWPORT_SIZE = 2073600;

    static constexpr int SCENE_SIZE = 200000;
    static constexpr int CENTER_OFFSET = 10000;
    static constexpr int ZOOM_THRESHOLD_FACTOR = 4;

    void initSettings();
    void setScalingFilterImpl(ScalingFilter filter);
    void setFitModeImpl(ImageFitMode mode);
    bool imageFitsInternal() const;
    float currentScaleInternal() const;

    void initializeScene();
    void initializeTimers();
    void initializeScrollBars();
    void setupConnections();

    void adjustZoom(bool zoomIn, bool atCursor);
    void zoomAnchored(float newScale);
    void doZoom(float newScale);
    void setZoomAnchor(QPoint viewportPos);

    void fitNormal();
    void fitWidth();
    void fitWindow();
    void fitWindowStretch();
    void fitFree(float scale);
    void applyFitMode();
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

    void centerIfNecessary();
    void snapToEdges();
    void saveViewportPos();
    void applySavedViewportPos();
    void lockZoom();

    void updatePixmap(const QPixmap& newPixmap);
    void updatePixmap(QPixmap&& newPixmap);
    void swapToOriginalPixmap();
    Qt::TransformationMode selectTransformationMode();

    void reset();
    QPointF sceneRoundPos(QPointF scenePoint) const;
    QRectF sceneRoundRect(QRectF sceneRect) const;
};