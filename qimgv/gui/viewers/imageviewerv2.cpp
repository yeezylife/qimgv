#include "imageviewerv2.h"
#include <QApplication>
#include <QPainter>
#include <QProcess>
#include <QScreen>

ImageViewerV2::ImageViewerV2(QWidget* parent)
    : QGraphicsView(parent)
    , scene(nullptr)
    , pixmap(nullptr)
    , movie(nullptr)
    , animationTimer(nullptr)
    , scaleTimer(nullptr)
    , horizontalScroll(nullptr)
    , verticalScroll(nullptr)
    , scrollTimeLineX(nullptr)
    , scrollTimeLineY(nullptr)
    , mouseInteraction(MOUSE_NONE)
    , transparencyGrid(false)
    , expandImage(false)
    , smoothAnimatedImages(true)
    , smoothUpscaling(true)
    , forceFastScale(false)
    , keepFitMode(false)
    , loopPlayback(true)
    , mIsFullscreen(false)
    , scrollBarWorkaround(true)
    , useFixedZoomLevels(false)
    , trackpadDetection(true)
    , dragsEnabled(true)
    , wayland(false)
    , zoomStep(0.1f)
    , dpr(1.0f)
    , minScale(0.01f)
    , maxScale(500.0f)
    , fitWindowScale(0.125f)
    , fitWindowStretchScale(0.125f)
    , expandLimit(1.0f)
    , lockedScale(1.0f)
    , mViewLock(LOCK_NONE)
    , imageFitMode(FIT_WINDOW)
    , imageFitModeDefault(FIT_WINDOW)
    , mScalingFilter(QI_FILTER_BILINEAR)
    , zoomThreshold(ZOOM_THRESHOLD_FACTOR)
    , dragThreshold(10)
{
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAcceptDrops(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);

    if (qApp->platformName() == "wayland")
        wayland = true;

    dpr = static_cast<float>(devicePixelRatioF());
    zoomThreshold = static_cast<int>(dpr * ZOOM_THRESHOLD_FACTOR);

    initializeScene();
    initializeTimers();
    initializeScrollBars();
    setupConnections();

    checkboard = QPixmap(":res/icons/common/other/checkerboard.png");
    lastTouchpadScroll.start();

    initSettings();  // replaced virtual call readSettings()
}

ImageViewerV2::~ImageViewerV2() = default;

void ImageViewerV2::initializeScene()
{
    pixmapItem.setTransformationMode(Qt::SmoothTransformation);
    pixmapItem.setScale(1.0f);
    pixmapItem.setOffset(CENTER_OFFSET, CENTER_OFFSET);
    pixmapItem.setTransformOriginPoint(CENTER_OFFSET, CENTER_OFFSET);

    pixmapItemScaled.setScale(1.0f);
    pixmapItemScaled.setOffset(CENTER_OFFSET, CENTER_OFFSET);
    pixmapItemScaled.setTransformOriginPoint(CENTER_OFFSET, CENTER_OFFSET);

    scene = new QGraphicsScene();
    scene->setSceneRect(0, 0, SCENE_SIZE, SCENE_SIZE);
    scene->setBackgroundBrush(QColor(60, 60, 103));
    scene->addItem(&pixmapItem);
    scene->addItem(&pixmapItemScaled);
    pixmapItemScaled.hide();

    setScene(scene);
}

void ImageViewerV2::initializeTimers()
{
    animationTimer = new QTimer(this);
    animationTimer->setSingleShot(true);

    scaleTimer = new QTimer(this);
    scaleTimer->setSingleShot(true);
    scaleTimer->setInterval(80);

    scrollTimeLineX = new QTimeLine();
    scrollTimeLineX->setEasingCurve(QEasingCurve::OutSine);
    scrollTimeLineX->setDuration(ANIMATION_SPEED);
    scrollTimeLineX->setUpdateInterval(SCROLL_UPDATE_RATE);

    scrollTimeLineY = new QTimeLine();
    scrollTimeLineY->setEasingCurve(QEasingCurve::OutSine);
    scrollTimeLineY->setDuration(ANIMATION_SPEED);
    scrollTimeLineY->setUpdateInterval(SCROLL_UPDATE_RATE);
}

void ImageViewerV2::initializeScrollBars()
{
    horizontalScroll = horizontalScrollBar();
    verticalScroll = verticalScrollBar();
}

void ImageViewerV2::setupConnections()
{
    connect(scrollTimeLineX, &QTimeLine::frameChanged, this, &ImageViewerV2::scrollToX);
    connect(scrollTimeLineY, &QTimeLine::frameChanged, this, &ImageViewerV2::scrollToY);
    connect(scrollTimeLineX, &QTimeLine::finished, this, &ImageViewerV2::onScrollTimelineFinished);
    connect(scrollTimeLineY, &QTimeLine::finished, this, &ImageViewerV2::onScrollTimelineFinished);
    connect(animationTimer, &QTimer::timeout, this, &ImageViewerV2::onAnimationTimer, Qt::UniqueConnection);
    connect(scaleTimer, &QTimer::timeout, this, &ImageViewerV2::requestScaling);
    connect(settings, &Settings::settingsChanged, this, &ImageViewerV2::readSettings);
}

bool ImageViewerV2::eventFilter(QObject* obj, QEvent* ev)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    if (ev->type() == QEvent::DevicePixelRatioChange) {
        onDPRChanged();
    }
#endif
    return QGraphicsView::eventFilter(obj, ev);
}

void ImageViewerV2::onDPRChanged()
{
    float newDpr = static_cast<float>(devicePixelRatioF());
    if (dpr == newDpr)
        return;

    dpr = newDpr;
    zoomThreshold = static_cast<int>(dpr * ZOOM_THRESHOLD_FACTOR);

    if (pixmap) {
        pixmap->setDevicePixelRatio(dpr);
        pixmapItem.setPixmap(*pixmap);
        pixmapItem.show();
        pixmapItem.update();
        updateMinScale();
        applyFitMode();
        requestScaling();
        update();
    }
}

// ----------------------------------------------------------------------------
// Internal non-virtual helpers
// ----------------------------------------------------------------------------
void ImageViewerV2::initSettings()
{
    transparencyGrid = settings->transparencyGrid();
    smoothAnimatedImages = settings->smoothAnimatedImages();
    smoothUpscaling = settings->smoothUpscaling();
    expandImage = settings->expandImage();
    expandLimit = static_cast<float>(settings->expandLimit());

    if (expandLimit < 1.0f)
        expandLimit = maxScale;

    keepFitMode = settings->keepFitMode();
    imageFitModeDefault = settings->imageFitMode();
    zoomStep = settings->zoomStep();
    focusIn1to1 = settings->focusPointIn1to1Mode();
    trackpadDetection = settings->trackpadDetection();

    useFixedZoomLevels = settings->useFixedZoomLevels();
    if (useFixedZoomLevels) {
        zoomLevels.clear();
        zoomLevels.reserve(16);

        const auto levelsStr = settings->zoomLevels().split(',');
        for (const auto& s : levelsStr)
            zoomLevels.append(s.toFloat());

        std::sort(zoomLevels.begin(), zoomLevels.end());
    }

    onFullscreenModeChanged(mIsFullscreen);
    updateMinScale();
    setScalingFilterImpl(settings->scalingFilter());
    setFitModeImpl(imageFitModeDefault);
}

void ImageViewerV2::setScalingFilterImpl(ScalingFilter filter)
{
    if (mScalingFilter == filter)
        return;

    mScalingFilter = filter;
    pixmapItem.setTransformationMode(selectTransformationMode());

    if (mScalingFilter == QI_FILTER_NEAREST)
        swapToOriginalPixmap();

    requestScaling();
}

void ImageViewerV2::setFitModeImpl(ImageFitMode mode)
{
    if (scaleTimer->isActive())
        scaleTimer->stop();

    stopPosAnimation();
    imageFitMode = mode;
    applyFitMode();
    requestScaling();
}

bool ImageViewerV2::imageFitsInternal() const
{
    if (!pixmap)
        return true;

    return (pixmap->width() <= static_cast<int>(static_cast<float>(viewport()->width()) * dpr) &&
            pixmap->height() <= static_cast<int>(static_cast<float>(viewport()->height()) * dpr));
}

float ImageViewerV2::currentScaleInternal() const
{
    return static_cast<float>(pixmapItem.scale());
}

// ----------------------------------------------------------------------------
// Virtual implementations (calling internal non-virtual functions)
// ----------------------------------------------------------------------------
void ImageViewerV2::readSettings()
{
    initSettings();
}

void ImageViewerV2::setScalingFilter(ScalingFilter filter)
{
    setScalingFilterImpl(filter);
}

void ImageViewerV2::setFitMode(ImageFitMode mode)
{
    setFitModeImpl(mode);
}

bool ImageViewerV2::imageFits() const
{
    return imageFitsInternal();
}

float ImageViewerV2::currentScale() const
{
    return currentScaleInternal();
}

void ImageViewerV2::onFullscreenModeChanged(bool mode)
{
    mIsFullscreen = mode;
    QColor bgColor = mode ? settings->colorScheme().background_fullscreen
                          : settings->colorScheme().background;
    bgColor.setAlphaF(mode ? 1.0f : static_cast<float>(settings->backgroundOpacity()));
    scene->setBackgroundBrush(bgColor);
}

// ============================================================================
// Animation Control
// ============================================================================
void ImageViewerV2::startAnimation()
{
    if (movie && movie->frameCount() > 1) {
        stopAnimation();
        emit animationPaused(false);
        animationTimer->start(movie->nextFrameDelay());
    }
}

void ImageViewerV2::stopAnimation()
{
    if (movie) {
        emit animationPaused(true);
        animationTimer->stop();
    }
}

void ImageViewerV2::pauseResume()
{
    if (movie) {
        if (animationTimer->isActive())
            stopAnimation();
        else
            startAnimation();
    }
}

void ImageViewerV2::onAnimationTimer()
{
    if (!movie)
        return;

    if (movie->currentFrameNumber() == movie->frameCount() - 1) {
        if (!loopPlayback) {
            emit animationPaused(true);
            emit playbackFinished();
            return;
        }
        movie->jumpToFrame(0);
    } else {
        if (!movie->jumpToNextFrame()) {
            stopAnimation();
            return;
        }
    }

    emit frameChanged(movie->currentFrameNumber());
    auto newFrame = std::make_unique<QPixmap>(movie->currentPixmap());
    updatePixmap(std::move(newFrame));
    animationTimer->start(movie->nextFrameDelay());
}

void ImageViewerV2::nextFrame()
{
    if (!movie)
        return;

    int next = (movie->currentFrameNumber() == movie->frameCount() - 1)
               ? 0 : movie->currentFrameNumber() + 1;
    showAnimationFrame(next);
}

void ImageViewerV2::prevFrame()
{
    if (!movie)
        return;

    int prev = (movie->currentFrameNumber() == 0)
               ? movie->frameCount() - 1 : movie->currentFrameNumber() - 1;
    showAnimationFrame(prev);
}

bool ImageViewerV2::showAnimationFrame(int frame)
{
    if (!movie || frame < 0 || frame >= movie->frameCount())
        return false;

    if (movie->currentFrameNumber() == frame)
        return true;

    if (frame < movie->currentFrameNumber())
        movie->jumpToFrame(0);

    while (frame != movie->currentFrameNumber()) {
        if (!movie->jumpToNextFrame())
            break;
    }

    emit frameChanged(movie->currentFrameNumber());
    auto newFrame = std::make_unique<QPixmap>(movie->currentPixmap());
    updatePixmap(std::move(newFrame));
    return true;
}

// ============================================================================
// Display Operations
// ============================================================================
void ImageViewerV2::updatePixmap(std::unique_ptr<QPixmap> newPixmap)
{
    pixmap = std::move(newPixmap);
    pixmap->setDevicePixelRatio(dpr);
    pixmapItem.setPixmap(*pixmap);
    pixmapItem.show();
    pixmapItem.update();
}

void ImageViewerV2::showAnimation(const std::shared_ptr<QMovie>& animation)
{
    if (!animation || !animation->isValid())
        return;

    reset();
    movie = animation;
    movie->jumpToFrame(0);

    Qt::TransformationMode mode = smoothAnimatedImages ? Qt::SmoothTransformation : Qt::FastTransformation;
    pixmapItem.setTransformationMode(mode);

    auto newFrame = std::make_unique<QPixmap>(movie->currentPixmap());
    updatePixmap(std::move(newFrame));

    emit durationChanged(movie->frameCount());
    emit frameChanged(0);

    updateMinScale();

    if (!keepFitMode || imageFitMode == FIT_FREE)
        imageFitMode = imageFitModeDefault;

    if (mViewLock == LOCK_NONE) {
        applyFitMode();
    } else {
        imageFitMode = FIT_FREE;
        fitFree(lockedScale);
        if (mViewLock == LOCK_ALL)
            applySavedViewportPos();
    }

    startAnimation();
}

void ImageViewerV2::showImage(std::unique_ptr<QPixmap> newPixmap)
{
    reset();

    if (!newPixmap)
        return;

    pixmapItemScaled.hide();
    pixmap = std::move(newPixmap);
    pixmap->setDevicePixelRatio(dpr);
    pixmapItem.setPixmap(*pixmap);

    Qt::TransformationMode mode = (mScalingFilter == QI_FILTER_NEAREST)
                                  ? Qt::FastTransformation : Qt::SmoothTransformation;
    pixmapItem.setTransformationMode(mode);
    pixmapItem.show();

    updateMinScale();

    if (!keepFitMode || imageFitMode == FIT_FREE)
        imageFitMode = imageFitModeDefault;

    if (mViewLock == LOCK_NONE) {
        applyFitMode();
    } else {
        imageFitMode = FIT_FREE;
        fitFree(lockedScale);
        if (mViewLock == LOCK_ALL)
            applySavedViewportPos();
    }

    requestScaling();
    update();
}

void ImageViewerV2::reset()
{
    stopPosAnimation();
    pixmapItemScaled.setPixmap(QPixmap());
    pixmapScaled = QPixmap();
    pixmapItem.setPixmap(QPixmap());
    pixmapItem.setScale(1.0f);
    pixmapItem.setOffset(CENTER_OFFSET, CENTER_OFFSET);
    pixmap.reset();
    stopAnimation();
    movie = nullptr;
    centerOn(CENTER_OFFSET, CENTER_OFFSET);
    viewport()->update();
}

void ImageViewerV2::closeImage()
{
    reset();
}

void ImageViewerV2::setScaledPixmap(const QPixmap& newFrame)
{
    if (!movie && newFrame.size() != scaledSizeR() * dpr)
        return;

    pixmapScaled = newFrame;
    pixmapScaled.setDevicePixelRatio(dpr);
    pixmapItemScaled.setPixmap(pixmapScaled);
    pixmapItem.hide();
    pixmapItemScaled.show();
}

bool ImageViewerV2::isDisplaying() const
{
    return (pixmap != nullptr);
}

// ============================================================================
// Scroll Operations
// ============================================================================
void ImageViewerV2::scrollUp()
{
    scroll(0, -DEFAULT_SCROLL_DISTANCE, true);
}

void ImageViewerV2::scrollDown()
{
    scroll(0, DEFAULT_SCROLL_DISTANCE, true);
}

void ImageViewerV2::scrollLeft()
{
    scroll(-DEFAULT_SCROLL_DISTANCE, 0, true);
}

void ImageViewerV2::scrollRight()
{
    scroll(DEFAULT_SCROLL_DISTANCE, 0, true);
}

void ImageViewerV2::scroll(int dx, int dy, bool animated)
{
    if (animated)
        scrollSmooth(dx, dy);
    else
        scrollPrecise(dx, dy);
}

void ImageViewerV2::scrollSmooth(int dx, int dy)
{
    if (dx) {
        int current = horizontalScroll->value();
        int newEnd = current + dx;
        bool redirect = false;

        if ((newEnd < current && current < scrollTimeLineX->endFrame()) ||
            (newEnd > current && current > scrollTimeLineX->endFrame())) {
            redirect = true;
        }

        if (scrollTimeLineX->state() == QTimeLine::Running && !redirect)
            newEnd = scrollTimeLineX->endFrame() + dx;

        scrollTimeLineX->stop();
        scrollTimeLineX->setFrameRange(current, newEnd);
        scrollTimeLineX->start();
    }

    if (dy) {
        int current = verticalScroll->value();
        int newEnd = current + dy;
        bool redirect = false;

        if ((newEnd < current && current < scrollTimeLineY->endFrame()) ||
            (newEnd > current && current > scrollTimeLineY->endFrame())) {
            redirect = true;
        }

        if (scrollTimeLineY->state() == QTimeLine::Running && !redirect)
            newEnd = scrollTimeLineY->endFrame() + dy;

        scrollTimeLineY->stop();
        scrollTimeLineY->setFrameRange(current, newEnd);
        scrollTimeLineY->start();
    }

    saveViewportPos();
}

void ImageViewerV2::scrollPrecise(int dx, int dy)
{
    stopPosAnimation();
    horizontalScroll->setValue(horizontalScroll->value() + dx);
    verticalScroll->setValue(verticalScroll->value() + dy);
    centerIfNecessary();
    snapToEdges();
    saveViewportPos();
}

void ImageViewerV2::scrollToX(int x)
{
    horizontalScroll->setValue(x);
    centerIfNecessary();
    snapToEdges();
    viewport()->update();
}

void ImageViewerV2::scrollToY(int y)
{
    verticalScroll->setValue(y);
    centerIfNecessary();
    snapToEdges();
    viewport()->update();
}

void ImageViewerV2::onScrollTimelineFinished()
{
    saveViewportPos();
}

void ImageViewerV2::stopPosAnimation()
{
    if (scrollTimeLineX->state() == QTimeLine::Running)
        scrollTimeLineX->stop();
    if (scrollTimeLineY->state() == QTimeLine::Running)
        scrollTimeLineY->stop();
}

// ============================================================================
// Settings and Filters
// ============================================================================
void ImageViewerV2::toggleTransparencyGrid()
{
    transparencyGrid = !transparencyGrid;
    scene->update();
}

void ImageViewerV2::setLoopPlayback(bool mode)
{
    if (movie && mode && loopPlayback != mode)
        startAnimation();
    loopPlayback = mode;
}

void ImageViewerV2::setFilterNearest()
{
    setScalingFilterImpl(QI_FILTER_NEAREST);
}

void ImageViewerV2::setFilterBilinear()
{
    setScalingFilterImpl(QI_FILTER_BILINEAR);
}

Qt::TransformationMode ImageViewerV2::selectTransformationMode()
{
    if (forceFastScale)
        return Qt::FastTransformation;

    if (movie) {
        if (!smoothAnimatedImages || (pixmapItem.scale() > 1.0f && !smoothUpscaling))
            return Qt::FastTransformation;
    } else {
        if ((pixmapItem.scale() > 1.0f && !smoothUpscaling) || mScalingFilter == QI_FILTER_NEAREST)
            return Qt::FastTransformation;
    }

    return Qt::SmoothTransformation;
}

void ImageViewerV2::setExpandImage(bool mode)
{
    expandImage = mode;
    updateMinScale();
    applyFitMode();
    requestScaling();
}

void ImageViewerV2::show()
{
    setMouseTracking(false);
    QGraphicsView::show();
    setMouseTracking(true);
}

void ImageViewerV2::hide()
{
    setMouseTracking(false);
    QWidget::hide();
}

void ImageViewerV2::requestScaling()
{
    if (!pixmap)
        return;

    const float scale = pixmapItem.scale();

    if (scale == 1.0f || movie)
        return;

    if (!smoothUpscaling && scale >= 1.0f)
        return;

    if (scaleTimer->isActive())
        scaleTimer->stop();

    if (scale < FAST_SCALE_THRESHOLD)
        emit scalingRequested(scaledSizeR() * dpr, mScalingFilter);
}

void ImageViewerV2::enableDrags()
{
    dragsEnabled = true;
}

void ImageViewerV2::disableDrags()
{
    dragsEnabled = false;
}

// ============================================================================
// Zoom Operations (with common helper)
// ============================================================================
void ImageViewerV2::zoomIn()
{
    adjustZoom(true, false);
}

void ImageViewerV2::zoomInCursor()
{
    adjustZoom(true, true);
}

void ImageViewerV2::zoomOut()
{
    adjustZoom(false, false);
}

void ImageViewerV2::zoomOutCursor()
{
    adjustZoom(false, true);
}

void ImageViewerV2::adjustZoom(bool zoomIn, bool atCursor)
{
    if (atCursor && underMouse())
        setZoomAnchor(mapFromGlobal(cursor().pos()));
    else
        setZoomAnchor(viewport()->rect().center());

    const float current = currentScaleInternal();

    float newScale = zoomIn
        ? current * (1.0f + zoomStep)
        : current * (1.0f - zoomStep);

    if (useFixedZoomLevels && !zoomLevels.isEmpty()) {
        if (zoomIn) {
            if (current < zoomLevels.first()) {
                newScale = qMin(current * (1.0f + zoomStep), zoomLevels.first());
            } else if (current >= zoomLevels.last()) {
                newScale = current * (1.0f + zoomStep);
            } else {
                for (float level : zoomLevels) {
                    if (current < level) {
                        newScale = level;
                        break;
                    }
                }
            }
        } else {
            if (current > zoomLevels.last()) {
                newScale = qMax(zoomLevels.last(), current * (1.0f - zoomStep));
            } else if (current <= zoomLevels.first()) {
                newScale = current * (1.0f - zoomStep);
            } else {
                for (int i = zoomLevels.size() - 1; i >= 0; --i) {
                    if (current > zoomLevels[i]) {
                        newScale = zoomLevels[i];
                        break;
                    }
                }
            }
        }
    }

    zoomAnchored(newScale);
    centerIfNecessary();
    snapToEdges();

    imageFitMode = FIT_FREE;

    if (pixmapItem.scale() == fitWindowScale)
        imageFitMode = FIT_WINDOW;
    else if (pixmapItem.scale() == fitWindowStretchScale)
        imageFitMode = FIT_WINDOW_STRETCH;
}

void ImageViewerV2::setZoomAnchor(QPoint viewportPos)
{
    zoomAnchor = QPair<QPointF, QPoint>(
        pixmapItem.mapFromScene(mapToScene(viewportPos)),
        viewportPos
    );
}

void ImageViewerV2::zoomAnchored(float newScale)
{
    const float current = currentScaleInternal();
    if (current == newScale)
        return;

    const QPoint viewportCenter = viewport()->rect().center();
    const QPointF sceneCenter = mapToScene(viewportCenter);

    doZoom(newScale);

    const QPointF anchorScene =
        pixmapItem.mapToScene(zoomAnchor.first);

    const QPoint mapped =
        mapFromScene(anchorScene);

    const QPointF diff =
        zoomAnchor.second - mapped;

    centerOn(sceneCenter - diff);

    requestScaling();
}

void ImageViewerV2::doZoom(float newScale)
{
    if (!pixmap)
        return;

    newScale = qBound(minScale, newScale, maxScale);

    auto tl = pixmapItem.sceneBoundingRect().topLeft().toPoint();
    pixmapItem.setOffset(tl);
    pixmapItem.setScale(newScale);
    pixmapItem.setTransformationMode(selectTransformationMode());
    swapToOriginalPixmap();

    emit scaleChanged(newScale);
}

void ImageViewerV2::swapToOriginalPixmap()
{
    if (!pixmap || !pixmapItemScaled.isVisible())
        return;

    pixmapItemScaled.hide();
    pixmapItemScaled.setPixmap(QPixmap());
    pixmapScaled = QPixmap();
    pixmapItem.show();
}

// ============================================================================
// Fit Mode Operations
// ============================================================================
void ImageViewerV2::updateFitWindowScale()
{
    float scaleFitX = static_cast<float>(viewport()->width()) * dpr / static_cast<float>(pixmap->width());
    float scaleFitY = static_cast<float>(viewport()->height()) * dpr / static_cast<float>(pixmap->height());

    fitWindowScale = qMin(scaleFitX, scaleFitY);

    if (expandImage && fitWindowScale > expandLimit)
        fitWindowScale = expandLimit;
}

void ImageViewerV2::updateFitWindowStretchScale()
{
    if (!pixmap)
        return;

    fitWindowStretchScale = static_cast<float>(viewport()->height()) * dpr / static_cast<float>(pixmap->height());

    if (expandImage && fitWindowStretchScale > expandLimit)
        fitWindowStretchScale = expandLimit;
}

void ImageViewerV2::updateMinScale()
{
    if (!pixmap)
        return;

    updateFitWindowScale();
    updateFitWindowStretchScale();

    if (settings->unlockMinZoom()) {
        if (!pixmap->isNull())
            minScale = static_cast<float>(qMax(10. / pixmap->width(), 10. / pixmap->height()));
        else
            minScale = 1.0f;
    } else {
        minScale = imageFitsInternal() ? 1.0f : fitWindowScale;
    }

    if (mViewLock != LOCK_NONE && lockedScale < minScale)
        minScale = lockedScale;
}

void ImageViewerV2::fitWidth()
{
    if (!pixmap)
        return;

    float scaleX = static_cast<float>(viewport()->width()) * dpr / static_cast<float>(pixmap->width());

    if (!expandImage && scaleX > 1.0f)
        scaleX = 1.0f;
    if (scaleX > expandLimit)
        scaleX = expandLimit;

    if (currentScaleInternal() != scaleX) {
        swapToOriginalPixmap();
        doZoom(scaleX);
    }

    centerIfNecessary();

    if (scaledSizeR().height() > viewport()->height()) {
        QPointF centerTarget = mapToScene(viewport()->rect()).boundingRect().center();
        centerTarget.setY(0);
        centerOn(centerTarget);
    }

    snapToEdges();
}

void ImageViewerV2::fitWindow()
{
    if (!pixmap)
        return;

    if (imageFitsInternal() && !expandImage) {
        fitNormal();
    } else {
        if (currentScaleInternal() != fitWindowScale) {
            swapToOriginalPixmap();
            doZoom(fitWindowScale);
        }

        if (scrollBarWorkaround) {
            scrollBarWorkaround = false;
            QTimer::singleShot(0, this, &ImageViewerV2::centerOnPixmap);
        } else {
            centerOnPixmap();
        }
    }
}

void ImageViewerV2::fitNormal()
{
    fitFree(1.0f);
}

void ImageViewerV2::fitWindowStretch()
{
    if (!pixmap)
        return;

    updateFitWindowStretchScale();

    if (currentScaleInternal() != fitWindowStretchScale) {
        swapToOriginalPixmap();
        doZoom(fitWindowStretchScale);
    }

    if (scrollBarWorkaround) {
        scrollBarWorkaround = false;
        QTimer::singleShot(0, this, &ImageViewerV2::centerOnPixmap);
    } else {
        centerOnPixmap();
    }
}

void ImageViewerV2::fitFree(float scale)
{
    if (!pixmap)
        return;

    if (focusIn1to1 == FOCUS_TOP) {
        doZoom(scale);
        centerIfNecessary();
        if (scaledSizeR().height() > viewport()->height()) {
            QPointF centerTarget = pixmapItem.sceneBoundingRect().center();
            centerTarget.setY(0);
            centerOn(centerTarget);
        }
        snapToEdges();
    } else {
        if (focusIn1to1 == FOCUS_CENTER)
            setZoomAnchor(viewport()->rect().center());
        else
            setZoomAnchor(mapFromGlobal(cursor().pos()));

        zoomAnchored(scale);
        centerIfNecessary();
        snapToEdges();
    }
}

void ImageViewerV2::applyFitMode()
{
    switch (imageFitMode) {
        case FIT_ORIGINAL:       fitNormal(); break;
        case FIT_WIDTH:          fitWidth(); break;
        case FIT_WINDOW:         fitWindow(); break;
        case FIT_WINDOW_STRETCH: fitWindowStretch(); break;
        default: break;
    }
}

void ImageViewerV2::setFitOriginal()
{
    setFitModeImpl(FIT_ORIGINAL);
}

void ImageViewerV2::setFitWidth()
{
    setFitModeImpl(FIT_WIDTH);
}

void ImageViewerV2::setFitWindow()
{
    setFitModeImpl(FIT_WINDOW);
}

void ImageViewerV2::setFitWindowStretch()
{
    setFitModeImpl(FIT_WINDOW_STRETCH);
}

void ImageViewerV2::centerOnPixmap()
{
    const QRectF imgRect = pixmapItem.sceneBoundingRect();

    const QRectF vport =
        mapToScene(viewport()->rect()).boundingRect();

    const qreal x =
        pixmapItem.offset().x() - (vport.width() - imgRect.width()) * 0.5;

    const qreal y =
        pixmapItem.offset().y() - (vport.height() - imgRect.height()) * 0.5;

    horizontalScroll->setValue(qRound(x));
    verticalScroll->setValue(qRound(y));
}

void ImageViewerV2::centerIfNecessary()
{
    if (!pixmap)
        return;

    const QSize sz = scaledSizeR();

    const QRectF imgRect = pixmapItem.sceneBoundingRect();
    const QRectF vport =
        mapToScene(viewport()->rect()).boundingRect();

    if (sz.width() <= viewport()->width()) {
        const qreal x =
            pixmapItem.offset().x() - (vport.width() - imgRect.width()) * 0.5;

        horizontalScroll->setValue(qRound(x));
    }

    if (sz.height() <= viewport()->height()) {
        const qreal y =
            pixmapItem.offset().y() - (vport.height() - imgRect.height()) * 0.5;

        verticalScroll->setValue(qRound(y));
    }
}

void ImageViewerV2::snapToEdges()
{
    const QRect imgRect = scaledRectR();

    const QRectF vportScene =
        mapToScene(viewport()->rect()).boundingRect();

    const QPointF centerTarget = vportScene.center();

    qreal xShift = 0.0;
    qreal yShift = 0.0;

    const int viewW = viewport()->width();
    const int viewH = viewport()->height();

    if (imgRect.width() > viewW) {
        if (imgRect.left() > 0)
            xShift = imgRect.left();
        else if (imgRect.right() < viewW)
            xShift = imgRect.right() - viewW;
    }

    if (imgRect.height() > viewH) {
        if (imgRect.top() > 0)
            yShift = imgRect.top();
        else if (imgRect.bottom() < viewH)
            yShift = imgRect.bottom() - viewH;
    }

    centerOn(centerTarget + QPointF(xShift, yShift));
}

// ============================================================================
// View Lock Operations
// ============================================================================
void ImageViewerV2::toggleLockZoom()
{
    if (!isDisplaying())
        return;

    if (mViewLock != LOCK_ZOOM) {
        mViewLock = LOCK_ZOOM;
        lockZoom();
    } else {
        mViewLock = LOCK_NONE;
    }
}

bool ImageViewerV2::lockZoomEnabled() const
{
    return (mViewLock == LOCK_ZOOM);
}

void ImageViewerV2::lockZoom()
{
    lockedScale = static_cast<float>(pixmapItem.scale());
    imageFitMode = FIT_FREE;
    saveViewportPos();
}

void ImageViewerV2::toggleLockView()
{
    if (!isDisplaying())
        return;

    if (mViewLock != LOCK_ALL) {
        mViewLock = LOCK_ALL;
        lockZoom();
        saveViewportPos();
    } else {
        mViewLock = LOCK_NONE;
    }
}

bool ImageViewerV2::lockViewEnabled() const
{
    return (mViewLock == LOCK_ALL);
}

void ImageViewerV2::saveViewportPos()
{
    if (mViewLock != LOCK_ALL)
        return;

    const QRectF itemRect = pixmapItem.sceneBoundingRect();

    const QPointF sceneCenter =
        mapToScene(viewport()->rect().center());

    savedViewportPos.setX(qBound(
        0.0,
        (sceneCenter.x() - itemRect.left()) / itemRect.width(),
        1.0));

    savedViewportPos.setY(qBound(
        0.0,
        (sceneCenter.y() - itemRect.top()) / itemRect.height(),
        1.0));
}

void ImageViewerV2::applySavedViewportPos()
{
    const QRectF itemRect = pixmapItem.sceneBoundingRect();

    const QPointF newScenePos(
        itemRect.left() + itemRect.width() * savedViewportPos.x(),
        itemRect.top() + itemRect.height() * savedViewportPos.y()
    );

    centerOn(newScenePos);
    centerIfNecessary();
    snapToEdges();
}

// ============================================================================
// Mouse Event Handlers
// ============================================================================
void ImageViewerV2::mousePressEvent(QMouseEvent* event)
{
    if (!pixmap) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    mouseMoveStartPos = event->pos();
    mousePressPos = mouseMoveStartPos;

    if (event->button() & Qt::RightButton) {
        setZoomAnchor(event->pos());
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void ImageViewerV2::mouseMoveEvent(QMouseEvent* event)
{
    QGraphicsView::mouseMoveEvent(event);

    if (!pixmap || mouseInteraction == MOUSE_DRAG || mouseInteraction == MOUSE_WHEEL_ZOOM)
        return;

    if (event->buttons() & Qt::LeftButton) {
        if (mouseInteraction == MOUSE_NONE) {
            if (scaledImageFits()) {
                if (dragsEnabled)
                    mouseInteraction = MOUSE_DRAG_BEGIN;
            } else {
                mouseInteraction = MOUSE_PAN;
                if (cursor().shape() != Qt::ClosedHandCursor)
                    setCursor(Qt::ClosedHandCursor);
            }
        }

        if (mouseInteraction == MOUSE_DRAG_BEGIN) {
            if ((abs(mousePressPos.x() - event->pos().x()) > dragThreshold) ||
                (abs(mousePressPos.y() - event->pos().y()) > dragThreshold)) {
                mouseInteraction = MOUSE_NONE;
                emit draggedOut();
            }
        }

        if (mouseInteraction == MOUSE_PAN) {
            mousePan(event);
        }
        return;
    }

    if (event->buttons() & Qt::RightButton) {
        if (mouseInteraction == MOUSE_ZOOM ||
            static_cast<float>(std::abs(mousePressPos.y() - event->pos().y())) >
                static_cast<float>(zoomThreshold) / dpr) {
            if (cursor().shape() != Qt::SizeVerCursor)
                setCursor(Qt::SizeVerCursor);

            mouseInteraction = MOUSE_ZOOM;

            if (viewport()->width() * viewport()->height() > LARGE_VIEWPORT_SIZE)
                forceFastScale = true;

            mouseMoveZoom(event);
        }
        return;
    }

    event->ignore();
}

void ImageViewerV2::mouseReleaseEvent(QMouseEvent* event)
{
    unsetCursor();

    if (forceFastScale) {
        forceFastScale = false;
        pixmapItem.setTransformationMode(selectTransformationMode());
    }

    if (!pixmap || mouseInteraction == MOUSE_NONE) {
        QGraphicsView::mouseReleaseEvent(event);
        event->ignore();
    }

    mouseInteraction = MOUSE_NONE;
}

void ImageViewerV2::mousePan(QMouseEvent* event)
{
    if (scaledImageFits())
        return;

    mouseMoveStartPos -= event->pos();
    scroll(mouseMoveStartPos.x(), mouseMoveStartPos.y(), false);
    mouseMoveStartPos = event->pos();
    saveViewportPos();
}

void ImageViewerV2::mouseMoveZoom(QMouseEvent* event)
{
    const float stepMultiplier = 0.003f;
    int moveDistance = mouseMoveStartPos.y() - event->pos().y();
    float newScale = currentScaleInternal() * (1.0f + stepMultiplier * static_cast<float>(moveDistance) * dpr);
    mouseMoveStartPos = event->pos();
    imageFitMode = FIT_FREE;

    zoomAnchored(newScale);
    centerIfNecessary();
    snapToEdges();

    if (pixmapItem.scale() == fitWindowScale)
        imageFitMode = FIT_WINDOW;
    else if (pixmapItem.scale() == fitWindowStretchScale)
        imageFitMode = FIT_WINDOW_STRETCH;
}

void ImageViewerV2::wheelEvent(QWheelEvent* event)
{
#ifdef __APPLE__
    if (event->phase() == Qt::ScrollBegin || event->phase() == Qt::ScrollEnd) {
        event->accept();
        return;
    }
#endif

    if (event->buttons() & Qt::RightButton) {
        handleWheelZoom(event);
        return;
    }

    if (event->modifiers() != Qt::NoModifier) {
        event->ignore();
        QGraphicsView::wheelEvent(event);
        return;
    }

    QPoint angleDelta = event->angleDelta();

    bool isWheel = true;
    if (trackpadDetection) {
        if (wayland)
            isWheel = (event->phase() == Qt::NoScrollPhase);
        else
            isWheel = angleDelta.y() &&
                      (abs(angleDelta.y()) >= 120 &&
                      !(angleDelta.y() % 60)) &&
                      lastTouchpadScroll.elapsed() > 250;
    }

    if (!isWheel) {
        handleTrackpadScroll(event);
    } else if (settings->imageScrolling() == SCROLL_BY_TRACKPAD_AND_WHEEL) {
        handleMouseWheelScroll(event);
    } else {
        event->ignore();
        QGraphicsView::wheelEvent(event);
    }

    saveViewportPos();
}

void ImageViewerV2::handleWheelZoom(QWheelEvent* event)
{
    event->accept();
    mouseInteraction = MOUSE_WHEEL_ZOOM;
    int delta = event->angleDelta().ry();
    if (delta > 0)
        zoomInCursor();
    else if (delta < 0)
        zoomOutCursor();
}

void ImageViewerV2::handleTrackpadScroll(QWheelEvent* event)
{
    lastTouchpadScroll.restart();
    event->accept();

    if (settings->imageScrolling() != SCROLL_NONE) {
        stopPosAnimation();
        QPoint pixelDelta = event->pixelDelta();   // 局部变量，会被使用
        QPoint angleDelta = event->angleDelta();
        int dx = abs(angleDelta.x()) > abs(pixelDelta.x()) ? angleDelta.x() : pixelDelta.x();
        int dy = abs(angleDelta.y()) > abs(pixelDelta.y()) ? angleDelta.y() : pixelDelta.y();
        horizontalScroll->setValue(qRound(horizontalScroll->value() - dx * TRACKPAD_SCROLL_MULTIPLIER));
        verticalScroll->setValue(qRound(verticalScroll->value() - dy * TRACKPAD_SCROLL_MULTIPLIER));
        centerIfNecessary();
        snapToEdges();
    }
}

void ImageViewerV2::handleMouseWheelScroll(QWheelEvent* event)
{
    QRect imgRect = scaledRectR();
    int delta = event->angleDelta().y();

    if ((delta < 0 && imgRect.bottom() > height() + 2) ||
        (delta > 0 && imgRect.top() < -2)) {
        event->accept();
        scroll(0, qRound(-delta * WHEEL_SCROLL_MULTIPLIER * settings->mouseScrollingSpeed()), true);
    } else {
        event->ignore();
    }
}

void ImageViewerV2::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    mousePressPos = mapFromGlobal(cursor().pos());

    if (parentWidget()->isVisible()) {
        stopPosAnimation();
        updateMinScale();

        if (imageFitMode == FIT_FREE || imageFitMode == FIT_ORIGINAL) {
            centerIfNecessary();
            snapToEdges();
        } else {
            applyFitMode();
        }

        update();

        if (scaleTimer->isActive())
            scaleTimer->stop();
        scaleTimer->start();

        saveViewportPos();
    }
}

void ImageViewerV2::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);
    qApp->processEvents();

    if (imageFitMode == FIT_ORIGINAL)
        applyFitMode();
}

void ImageViewerV2::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawBackground(painter, rect);

    if (!isDisplaying() || !transparencyGrid || !pixmap->hasAlphaChannel())
        return;

    painter->drawTiledPixmap(pixmapItem.sceneBoundingRect(), checkboard);
}

// ============================================================================
// Query Methods
// ============================================================================
ImageFitMode ImageViewerV2::fitMode() const
{
    return imageFitMode;
}

bool ImageViewerV2::scaledImageFits() const
{
    if (!pixmap)
        return true;

    QSize sz = scaledSizeR();
    return (sz.width() <= viewport()->width() && sz.height() <= viewport()->height());
}

ScalingFilter ImageViewerV2::scalingFilter() const
{
    return mScalingFilter;
}

QWidget* ImageViewerV2::widget()
{
    return this;
}

bool ImageViewerV2::hasAnimation() const
{
    return (movie != nullptr);
}

QSize ImageViewerV2::scaledSizeR() const
{
    if (!pixmap)
        return QSize(0, 0);

    const QRectF rect =
        pixmapItem.mapRectToScene(pixmapItem.boundingRect());

    return sceneRoundRect(rect).size().toSize();
}

QRect ImageViewerV2::scaledRectR() const
{
    const QRectF rect =
        pixmapItem.mapRectToScene(pixmapItem.boundingRect());

    const QPoint topLeft = mapFromScene(rect.topLeft());
    const QPoint bottomRight = mapFromScene(rect.bottomRight());

    return QRect(topLeft, bottomRight);
}

QSize ImageViewerV2::sourceSize() const
{
    if (!pixmap)
        return QSize(0, 0);
    return pixmap->size();
}

QPointF ImageViewerV2::sceneRoundPos(QPointF scenePoint) const
{
    return mapToScene(mapFromScene(scenePoint));
}

QRectF ImageViewerV2::sceneRoundRect(QRectF sceneRect) const
{
    return QRectF(sceneRoundPos(sceneRect.topLeft()), sceneRect.size());
}