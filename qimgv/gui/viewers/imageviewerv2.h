#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QElapsedTimer>
#include <QWheelEvent>
#include <QTimeLine>
#include <QScrollBar>
#include <QMovie>
#include <QColor>
#include <QTimer>
#include <QDebug>
#include <memory>
#include <cmath>
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
    ImageViewerV2(QWidget* parent = nullptr);
    ~ImageViewerV2();

    virtual ImageFitMode fitMode() const;
    virtual QRect scaledRectR() const;
    virtual float currentScale() const;
    virtual QSize sourceSize() const;
    virtual void showImage(std::unique_ptr<QPixmap> _pixmap);
    virtual void showAnimation(std::shared_ptr<QMovie> _animation);
    virtual void setScaledPixmap(const QPixmap& newFrame);
    virtual bool isDisplaying() const;
    virtual bool imageFits() const;
    virtual ScalingFilter scalingFilter() const;
    virtual QWidget *widget();

    bool scaledImageFits() const;
    bool hasAnimation() const;
    QSize scaledSizeR() const;

    void pauseResume();
    void enableDrags();
    void disableDrags();

signals:
    void scalingRequested(QSize, ScalingFilter);
    void scaleChanged(qreal);
    void sourceSizeChanged(QSize);
    void imageAreaChanged(QRect);
    void draggedOut();
    void playbackFinished();
    void animationPaused(bool);
    void frameChanged(int);
    void durationChanged(int);

public slots:
    virtual void setFitMode(ImageFitMode mode);
    virtual void setFitOriginal();
    virtual void setFitWidth();
    virtual void setFitWindow();
    virtual void setFitWindowStretch();
    virtual void zoomIn();
    virtual void zoomOut();
    virtual void zoomInCursor();
    virtual void zoomOutCursor();
    virtual void readSettings();
    virtual void scrollUp();
    virtual void scrollDown();
    virtual void scrollLeft();
    virtual void scrollRight();
    virtual void startAnimation();
    virtual void stopAnimation();
    virtual void closeImage();
    virtual void setExpandImage(bool mode);
    virtual void show();
    virtual void hide();
    virtual void setFilterNearest();
    virtual void setFilterBilinear();
    virtual void setScalingFilter(ScalingFilter filter);

    void setLoopPlayback(bool mode);
    void toggleTransparencyGrid();
    void nextFrame();
    void prevFrame();
    bool showAnimationFrame(int frame);
    void onFullscreenModeChanged(bool mode);
    void toggleLockZoom();
    bool lockZoomEnabled();
    void toggleLockView();
    bool lockViewEnabled();

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
    virtual void drawBackground(QPainter *painter, const QRectF &rect) override;
    virtual bool eventFilter(QObject *o, QEvent *ev) override;

private slots:
    void onAnimationTimer();
    void requestScaling();
    void scrollToX(int x);
    void scrollToY(int y);
    void centerOnPixmap();
    void onScrollTimelineFinished();
    void onDPRChanged();

private:
    // Scene and display components
    QGraphicsScene *scene;
    std::shared_ptr<QPixmap> pixmap;
    QPixmap pixmapScaled;
    std::shared_ptr<QMovie> movie;
    QGraphicsPixmapItem pixmapItem, pixmapItemScaled;
    QPixmap *checkboard;

    // Timers and scrollbars
    QTimer *animationTimer, *scaleTimer;
    QScrollBar *hs, *vs;
    QTimeLine *scrollTimeLineX, *scrollTimeLineY;
    QElapsedTimer lastTouchpadScroll;

    // Mouse interaction state
    QPoint mouseMoveStartPos, mousePressPos, drawPos;
    MouseInteractionState mouseInteraction;
    QPair<QPointF, QPoint> zoomAnchor;

    // Display settings
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

    // Zoom and scale settings
    QList<float> zoomLevels;
    float zoomStep;
    float dpr;
    float minScale, maxScale;
    float fitWindowScale, fitWindowStretchScale;
    float expandLimit, lockedScale;
    QPointF savedViewportPos;
    ViewLockMode mViewLock;
    ImageFitMode imageFitMode, imageFitModeDefault;
    ImageFocusPoint focusIn1to1;
    ScalingFilter mScalingFilter;

    // Constants
    static constexpr int SCROLL_UPDATE_RATE = 7;
    static constexpr int DEFAULT_SCROLL_DISTANCE = 240;
    static constexpr qreal TRACKPAD_SCROLL_MULTIPLIER = 0.7;
    static constexpr qreal WHEEL_SCROLL_MULTIPLIER = 2.0f;
    static constexpr int ANIMATION_SPEED = 150;
    static constexpr float FAST_SCALE_THRESHOLD = 1.0f;
    static constexpr int LARGE_VIEWPORT_SIZE = 2073600;

    int zoomThreshold;
    int dragThreshold;

    // Internal non-virtual helpers
    void initSettings();
    void setScalingFilterImpl(ScalingFilter filter);
    void setFitModeImpl(ImageFitMode mode);
    bool imageFitsInternal() const;
    float currentScaleInternal() const;

    // Initialization
    void initializeScene();
    void initializeTimers();
    void initializeScrollBars();
    void setupConnections();

    // Zoom operations
    void zoomAnchored(float newScale);
    void doZoom(float newScale);
    void doZoomIn(bool atCursor);
    void doZoomOut(bool atCursor);
    void setZoomAnchor(QPoint viewportPos);

    // Fit mode operations
    void fitNormal();
    void fitWidth();
    void fitWindow();
    void fitWindowStretch();
    void fitFree(float scale);
    void applyFitMode();
    void updateFitWindowScale();
    void updateFitWindowStretchScale();
    void updateMinScale();

    // Scroll operations
    void scroll(int dx, int dy, bool animated);
    void scrollSmooth(int dx, int dy);
    void scrollPrecise(int dx, int dy);
    void stopPosAnimation();

    // Mouse operations
    void mousePan(QMouseEvent *event);
    void mouseMoveZoom(QMouseEvent *event);

    // View operations
    void centerIfNecessary();
    void snapToEdges();
    void saveViewportPos();
    void applySavedViewportPos();
    void lockZoom();

    // Pixmap operations
    void updatePixmap(std::unique_ptr<QPixmap> newPixmap);
    void swapToOriginalPixmap();
    Qt::TransformationMode selectTransformationMode();

    // Utility functions
    void reset();
    QPointF sceneRoundPos(QPointF scenePoint) const;
    QRectF sceneRoundRect(QRectF sceneRect) const;
};