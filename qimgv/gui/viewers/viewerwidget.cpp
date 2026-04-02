/*
 * This widget combines ImageViewer / VideoPlayer.
 * Only one is displayed at a time.
 */

#include "viewerwidget.h"

ViewerWidget::ViewerWidget(QWidget *parent)
    : FloatingWidgetContainer(parent),
      imageViewer(nullptr),
      videoPlayer(nullptr),
      contextMenu(nullptr),
      videoControls(nullptr),
      currentWidget(UNSET),
      mInteractionEnabled(false),
      mWaylandCursorWorkaround(false),
      mIsFullscreen(false)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
#ifdef Q_OS_LINUX
    // we cant check cursor position on wayland until the mouse is moved
    // use this to skip cursor check once
    if(qgetenv("XDG_SESSION_TYPE") == "wayland")
        mWaylandCursorWorkaround = true;
#endif
    layout.setContentsMargins(0, 0, 0, 0);
    layout.setSpacing(0);
    this->setLayout(&layout);

    imageViewer = new ImageViewerV2(this);
    layout.addWidget(imageViewer);
    imageViewer->hide();

    connect(imageViewer, &ImageViewerV2::scalingRequested, this, &ViewerWidget::scalingRequested);
    connect(imageViewer, &ImageViewerV2::scaleChanged, this, &ViewerWidget::onScaleChanged);
    connect(imageViewer, &ImageViewerV2::playbackFinished, this, &ViewerWidget::onAnimationPlaybackFinished);
    connect(this, &ViewerWidget::toggleTransparencyGrid, imageViewer, &ImageViewerV2::toggleTransparencyGrid);
    connect(this, &ViewerWidget::setFilterNearest,       imageViewer, &ImageViewerV2::setFilterNearest);
    connect(this, &ViewerWidget::setFilterBilinear,      imageViewer, &ImageViewerV2::setFilterBilinear);
    connect(this, &ViewerWidget::setScalingFilter,       imageViewer, &ImageViewerV2::setScalingFilter);


    videoPlayer = new VideoPlayerInitProxy(this);
    layout.addWidget(videoPlayer);
    videoPlayer->hide();
    videoControls = new VideoControlsProxyWrapper(this);

    // tmp no wrapper
    zoomIndicator = new ZoomIndicatorOverlayProxy(this);
    clickZoneOverlay = new ClickZoneOverlay(this);

    connect(videoPlayer, &VideoPlayer::playbackFinished, this, &ViewerWidget::onVideoPlaybackFinished);

    connect(videoControls, &VideoControlsProxyWrapper::seekBackward,  this, &ViewerWidget::seekBackward);
    connect(videoControls, &VideoControlsProxyWrapper::seekForward, this, &ViewerWidget::seekForward);
    connect(videoControls, &VideoControlsProxyWrapper::seek,      this, &ViewerWidget::seek);

    enableImageViewer();
    setInteractionEnabled(true);

    connect(&cursorTimer, &QTimer::timeout, this, &ViewerWidget::hideCursor);

    connect(settings, &Settings::settingsChanged, this, &ViewerWidget::readSettings);
    readSettings();
}

QRect ViewerWidget::imageRect() {
    if(imageViewer && currentWidget == IMAGEVIEWER)
        return imageViewer->scaledRectR();
    return QRect(0,0,0,0);
}

float ViewerWidget::currentScale() {
    if(currentWidget == IMAGEVIEWER)
        return imageViewer->currentScale();
    return 1.0f;
}

QSize ViewerWidget::sourceSize() {
    if(currentWidget == IMAGEVIEWER)
        return imageViewer->sourceSize();
    return QSize(0,0);
}

// hide videoPlayer, show imageViewer
void ViewerWidget::enableImageViewer() {
    if(currentWidget == IMAGEVIEWER)
        return;

    disableVideoPlayer();

    videoControls->setMode(PLAYBACK_ANIMATION);

    // 防止重复连接（Qt::UniqueConnection）
    connect(imageViewer, &ImageViewerV2::durationChanged,
            videoControls, &VideoControlsProxyWrapper::setPlaybackDuration,
            Qt::UniqueConnection);

    connect(imageViewer, &ImageViewerV2::frameChanged,
            videoControls, &VideoControlsProxyWrapper::setPlaybackPosition,
            Qt::UniqueConnection);

    connect(imageViewer, &ImageViewerV2::animationPaused,
            videoControls, &VideoControlsProxyWrapper::onPlaybackPaused,
            Qt::UniqueConnection);

    imageViewer->show();
    currentWidget = IMAGEVIEWER;
}

// hide imageViewer, show videoPlayer
void ViewerWidget::enableVideoPlayer() {
    if(currentWidget == VIDEOPLAYER)
        return;

    disableImageViewer();

    videoControls->setMode(PLAYBACK_VIDEO);

    connect(videoPlayer, &VideoPlayer::durationChanged,
            videoControls, &VideoControlsProxyWrapper::setPlaybackDuration,
            Qt::UniqueConnection);

    connect(videoPlayer, &VideoPlayer::positionChanged,
            videoControls, &VideoControlsProxyWrapper::setPlaybackPosition,
            Qt::UniqueConnection);

    connect(videoPlayer, &VideoPlayer::videoPaused,
            videoControls, &VideoControlsProxyWrapper::onPlaybackPaused,
            Qt::UniqueConnection);

    videoPlayer->show();
    currentWidget = VIDEOPLAYER;
}

void ViewerWidget::disableImageViewer() {
    if(currentWidget == IMAGEVIEWER) {
        currentWidget = UNSET;
        imageViewer->closeImage();
        imageViewer->hide();
        zoomIndicator->hide();
        disconnect(imageViewer, &ImageViewerV2::durationChanged, videoControls, &VideoControlsProxyWrapper::setPlaybackDuration);
        disconnect(imageViewer, &ImageViewerV2::frameChanged,    videoControls, &VideoControlsProxyWrapper::setPlaybackPosition);
        disconnect(imageViewer, &ImageViewerV2::animationPaused, videoControls, &VideoControlsProxyWrapper::onPlaybackPaused);
    }
}

void ViewerWidget::disableVideoPlayer() {
    if(currentWidget == VIDEOPLAYER) {
        currentWidget = UNSET;
        //videoControls->hide();
        disconnect(videoPlayer, &VideoPlayer::durationChanged, videoControls, &VideoControlsProxyWrapper::setPlaybackDuration);
        disconnect(videoPlayer, &VideoPlayer::positionChanged, videoControls, &VideoControlsProxyWrapper::setPlaybackPosition);
        disconnect(videoPlayer, &VideoPlayer::videoPaused,     videoControls, &VideoControlsProxyWrapper::onPlaybackPaused);
        videoPlayer->setPaused(true);
        // even after calling hide() the player sends a few video frames
        // which paints over the imageviewer, causing corruption
        // so we do not HIDE it, but rather just cover it by imageviewer's widget
        // seems to work fine, might even feel a bit snappier
        if(!videoPlayer->isInitialized())
            videoPlayer->hide();
    }
}

void ViewerWidget::onScaleChanged(qreal scale) {
    if(!this->isVisible())
        return;
    if(scale != 1.0f) {
        zoomIndicator->setScale(scale);
        if(settings->zoomIndicatorMode() == ZoomIndicatorMode::INDICATOR_ENABLED)
            zoomIndicator->show();
        else if((settings->zoomIndicatorMode() == ZoomIndicatorMode::INDICATOR_AUTO))
            zoomIndicator->show(1500);
    } else {
        zoomIndicator->hide();
    }
}

void ViewerWidget::onVideoPlaybackFinished() {
    if(currentWidget == VIDEOPLAYER)
        emit playbackFinished();
}

void ViewerWidget::onAnimationPlaybackFinished() {
    if(currentWidget == IMAGEVIEWER)
        emit playbackFinished();
}

void ViewerWidget::setInteractionEnabled(bool mode) {
    if(mInteractionEnabled == mode)
        return;
    mInteractionEnabled = mode;
    if(mInteractionEnabled) {
        connect(this, &ViewerWidget::toggleLockZoom, imageViewer, &ImageViewerV2::toggleLockZoom);
        connect(this, &ViewerWidget::toggleLockView, imageViewer, &ImageViewerV2::toggleLockView);
        connect(this, &ViewerWidget::zoomIn,         imageViewer, &ImageViewerV2::zoomIn);
        connect(this, &ViewerWidget::zoomOut,        imageViewer, &ImageViewerV2::zoomOut);
        connect(this, &ViewerWidget::zoomInCursor,   imageViewer, &ImageViewerV2::zoomInCursor);
        connect(this, &ViewerWidget::zoomOutCursor,  imageViewer, &ImageViewerV2::zoomOutCursor);
        connect(this, &ViewerWidget::scrollUp,       imageViewer, &ImageViewerV2::scrollUp);
        connect(this, &ViewerWidget::scrollDown,     imageViewer, &ImageViewerV2::scrollDown);
        connect(this, &ViewerWidget::scrollLeft,     imageViewer, &ImageViewerV2::scrollLeft);
        connect(this, &ViewerWidget::scrollRight,    imageViewer, &ImageViewerV2::scrollRight);
        connect(this, &ViewerWidget::fitWindow,      imageViewer, &ImageViewerV2::setFitWindow);
        connect(this, &ViewerWidget::fitWidth,       imageViewer, &ImageViewerV2::setFitWidth);
        connect(this, &ViewerWidget::fitOriginal,    imageViewer, &ImageViewerV2::setFitOriginal);
        connect(this, &ViewerWidget::fitWindowStretch, imageViewer, &ImageViewerV2::setFitWindowStretch);
        connect(imageViewer, &ImageViewerV2::draggedOut, this, &ViewerWidget::draggedOut);
        imageViewer->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    } else {
        disconnect(this, &ViewerWidget::zoomIn,        imageViewer, &ImageViewerV2::zoomIn);
        disconnect(this, &ViewerWidget::zoomOut,       imageViewer, &ImageViewerV2::zoomOut);
        disconnect(this, &ViewerWidget::zoomInCursor,  imageViewer, &ImageViewerV2::zoomInCursor);
        disconnect(this, &ViewerWidget::zoomOutCursor, imageViewer, &ImageViewerV2::zoomOutCursor);
        disconnect(this, &ViewerWidget::scrollUp,      imageViewer, &ImageViewerV2::scrollUp);
        disconnect(this, &ViewerWidget::scrollDown,    imageViewer, &ImageViewerV2::scrollDown);
        disconnect(this, &ViewerWidget::scrollLeft,    imageViewer, &ImageViewerV2::scrollLeft);
        disconnect(this, &ViewerWidget::scrollRight,   imageViewer, &ImageViewerV2::scrollRight);
        disconnect(this, &ViewerWidget::fitWindow,     imageViewer, &ImageViewerV2::setFitWindow);
        disconnect(this, &ViewerWidget::fitWidth,      imageViewer, &ImageViewerV2::setFitWidth);
        disconnect(this, &ViewerWidget::fitOriginal,   imageViewer, &ImageViewerV2::setFitOriginal);
        disconnect(this, &ViewerWidget::fitWindowStretch, imageViewer, &ImageViewerV2::setFitWindowStretch);
        disconnect(imageViewer, &ImageViewerV2::draggedOut, this, &ViewerWidget::draggedOut);
        imageViewer->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        hideContextMenu();
    }
}

bool ViewerWidget::interactionEnabled() {
    return mInteractionEnabled;
}

bool ViewerWidget::showImage(const QPixmap& pixmap) {
    if(pixmap.isNull())
        return false;
    stopPlayback();
    videoControls->hide();
    enableImageViewer();
    imageViewer->showImage(pixmap);
    hideCursorTimed(false);
    return true;
}

bool ViewerWidget::showAnimation(const std::shared_ptr<QMovie>& movie) {
    if(!movie)
        return false;
    stopPlayback();
    enableImageViewer();
    imageViewer->showAnimation(movie);
    hideCursorTimed(false);
    return true;
}

bool ViewerWidget::showVideo(QString file) {
    stopPlayback();
    enableVideoPlayer();
    videoPlayer->showVideo(std::move(file));
    hideCursorTimed(false);
    return true;
}

void ViewerWidget::stopPlayback() {
    if(currentWidget == IMAGEVIEWER && imageViewer->hasAnimation())
        imageViewer->stopAnimation();
    if(currentWidget == VIDEOPLAYER) {
        // stopping is visibly slower
        //videoPlayer->stop();
        videoPlayer->setPaused(true);
    }
}

void ViewerWidget::startPlayback() {
    if(currentWidget == IMAGEVIEWER && imageViewer->hasAnimation())
        imageViewer->startAnimation();
    if(currentWidget == VIDEOPLAYER) {
        // stopping is visibly slower
        //videoPlayer->stop();
        videoPlayer->setPaused(false);
    }
}

void ViewerWidget::setFitMode(ImageFitMode mode) {
    if(mode == FIT_WINDOW)
        emit fitWindow();
    else if(mode == FIT_WIDTH)
        emit fitWidth();
    else if(mode == FIT_ORIGINAL)
        emit fitOriginal();
}

ImageFitMode ViewerWidget::fitMode() {
    return imageViewer->fitMode();
}

void ViewerWidget::onScalingFinished(const QPixmap& scaled) {
    // 修复：改为常引用，并移除 std::move
    imageViewer->setScaledPixmap(scaled);
}

void ViewerWidget::closeImage() {
    imageViewer->closeImage();
    videoPlayer->stop();
    showCursor();
}

void ViewerWidget::pauseResumePlayback() {
    if(currentWidget == VIDEOPLAYER)
        videoPlayer->pauseResume();
    else if(imageViewer->hasAnimation())
        imageViewer->pauseResume();
}

void ViewerWidget::seek(int pos) {
    if(currentWidget == VIDEOPLAYER) {
        videoPlayer->seek(pos);
    } else if(imageViewer->hasAnimation()) {
        imageViewer->stopAnimation();
        imageViewer->showAnimationFrame(pos);
    }
}

void ViewerWidget::seekRelative(int pos) {
    if(currentWidget == VIDEOPLAYER)
        videoPlayer->seekRelative(pos);
}

void ViewerWidget::seekBackward() {
    if(currentWidget == VIDEOPLAYER)
        videoPlayer->seekRelative(-10);
}

void ViewerWidget::seekForward() {
    if(currentWidget == VIDEOPLAYER)
        videoPlayer->seekRelative(10);
}

void ViewerWidget::frameStep() {
    if(currentWidget == VIDEOPLAYER)
        videoPlayer->frameStep();
    else if(imageViewer->hasAnimation()) {
        imageViewer->stopAnimation();
        imageViewer->nextFrame();
    }
}

void ViewerWidget::frameStepBack() {
    if(currentWidget == VIDEOPLAYER)
        videoPlayer->frameStepBack();
    else if(imageViewer->hasAnimation()) {
        imageViewer->stopAnimation();
        imageViewer->prevFrame();
    }
}

void ViewerWidget::toggleMute() {
    if(currentWidget == VIDEOPLAYER) {
        videoPlayer->setMuted(!videoPlayer->muted());
        videoControls->onVideoMuted(videoPlayer->muted());
    }
}

void ViewerWidget::volumeUp() {
    if(currentWidget == VIDEOPLAYER)
        videoPlayer->volumeUp();
}

void ViewerWidget::volumeDown() {
    if(currentWidget == VIDEOPLAYER) {
        videoPlayer->volumeDown();
    }
}

bool ViewerWidget::isDisplaying() {
    if(currentWidget == IMAGEVIEWER && imageViewer->isDisplaying())
        return true;
    if(currentWidget == VIDEOPLAYER)
        return true;
    return false;
}


bool ViewerWidget::lockZoomEnabled() {
    return imageViewer->lockZoomEnabled();
}

bool ViewerWidget::lockViewEnabled() {
    return imageViewer->lockViewEnabled();
}

ScalingFilter ViewerWidget::scalingFilter() {
    return imageViewer->scalingFilter();
}

void ViewerWidget::mousePressEvent(QMouseEvent *event) {
    hideContextMenu();
    event->ignore();
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent *event) {
    showCursor();
    hideCursorTimed(false);
    event->ignore();
}

void ViewerWidget::mouseMoveEvent(QMouseEvent *event) {
    mWaylandCursorWorkaround = false;

    const bool noButton =
        !(event->buttons() & Qt::LeftButton) &&
        !(event->buttons() & Qt::RightButton);

    if(noButton) {
        showCursor();
        hideCursorTimed(true);
    }

    if(currentWidget == VIDEOPLAYER || imageViewer->hasAnimation()) {
        const QPoint pos = event->position().toPoint();
        const QRect area = videoControlsArea();

        if(area.contains(pos))
            videoControls->show();
        else
            videoControls->hide();
    }

    event->ignore();
}

void ViewerWidget::hideCursorTimed(bool restartTimer) {
    if(restartTimer || !cursorTimer.isActive())
        cursorTimer.start(CURSOR_HIDE_TIMEOUT_MS);
}

void ViewerWidget::hideCursor() {
    cursorTimer.stop();

    if(!isDisplaying() || !isActiveWindow())
        return;

    if(contextMenu && contextMenu->isVisible())
        return;

    if(!settings->cursorAutohide())
        return;

    if(mWaylandCursorWorkaround) {
        setCursor(QCursor(Qt::BlankCursor));
        videoControls->hide();
        return;
    }

    const QPoint globalPos = QCursor::pos();
    const QPoint pos = mapFromGlobal(globalPos);

    if(clickZoneOverlay->leftZone().contains(pos) ||
       clickZoneOverlay->rightZone().contains(pos))
        return;

    // O(1) 替代 widgetAt 的 O(n) 遍历
    bool inTarget = false;

    if(imageViewer && imageViewer->viewport()) {
        const QPoint p = imageViewer->viewport()->mapFrom(this, pos);
        if(imageViewer->viewport()->rect().contains(p))
            inTarget = true;
    }

    if(!inTarget && videoPlayer && videoPlayer->getPlayer()) {
        auto *player = videoPlayer->getPlayer().get();
        const QPoint p = player->mapFrom(this, pos);
        if(player->rect().contains(p))
            inTarget = true;
    }

    if(inTarget) {
        if(!videoControls->isVisible() || !videoControlsArea().contains(pos)) {
            setCursor(QCursor(Qt::BlankCursor));
            videoControls->hide();
        }
    }
}

QRect ViewerWidget::videoControlsArea() {
    if(settings->panelEnabled() && settings->panelPosition() == PANEL_BOTTOM)
        return QRect(0, 0, width(), 160);

    return QRect(0, height() - 160, width(), 160);
}

// click zone input crutch
// --
// we can't process mouse events in the overlay
// cause they won't propagate to the ImageViewer, only to overlay's container (this widget)
// so we just grab them before they reach ImageViewer and do the needful
bool ViewerWidget::eventFilter(QObject *object, QEvent *event) {
    const auto type = event->type();

    if(type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick) {
        if(width() <= 250)
            return false;

        auto *mouseEvent = static_cast<QMouseEvent*>(event);

        if(mouseEvent->button() != Qt::LeftButton || mouseEvent->modifiers()) {
            clickZoneOverlay->disableHighlight();
            return false;
        }

        const QPoint pos = mouseEvent->position().toPoint();

        if(clickZoneOverlay->leftZone().contains(pos)) {
            clickZoneOverlay->setPressed(true);
            clickZoneOverlay->highlightLeft();
            imageViewer->disableDrags();
            actionManager->invokeAction("prevImage");
            return true;
        }

        if(clickZoneOverlay->rightZone().contains(pos)) {
            clickZoneOverlay->setPressed(true);
            clickZoneOverlay->highlightRight();
            imageViewer->disableDrags();
            actionManager->invokeAction("nextImage");
            return true;
        }
    }

    if(type == QEvent::ContextMenu) {
        clickZoneOverlay->disableHighlight();
        return false;
    }

    if(type == QEvent::MouseButtonRelease) {
        clickZoneOverlay->setPressed(false);
        imageViewer->enableDrags();
    }

    if(type == QEvent::MouseMove || type == QEvent::Enter) {
        QPoint pos;

        if(type == QEvent::MouseMove) {
            auto *mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->buttons())
                return false;
            pos = mouseEvent->position().toPoint();
        } else {
            auto *enterEvent = static_cast<QEnterEvent*>(event);
            pos = enterEvent->position().toPoint();
        }

        if(clickZoneOverlay->leftZone().contains(pos)) {
            clickZoneOverlay->setPressed(false);
            clickZoneOverlay->highlightLeft();
            setCursor(Qt::PointingHandCursor);
            return true;
        }

        if(clickZoneOverlay->rightZone().contains(pos)) {
            clickZoneOverlay->setPressed(false);
            clickZoneOverlay->highlightRight();
            setCursor(Qt::PointingHandCursor);
            return true;
        }

        clickZoneOverlay->disableHighlight();
        setCursor(Qt::ArrowCursor);
    }

    if(type == QEvent::Leave) {
        clickZoneOverlay->disableHighlight();
        setCursor(Qt::ArrowCursor);
    }

    return false;
}

void ViewerWidget::showCursor() {
    cursorTimer.stop();
    if(cursor().shape() == Qt::BlankCursor)
        setCursor(QCursor(Qt::ArrowCursor));
}

void ViewerWidget::showContextMenu() {
    QPoint pos = cursor().pos();
    showContextMenu(pos);
}

void ViewerWidget::showContextMenu(QPoint pos) {
    if(isVisible() && interactionEnabled()) {
        if(!contextMenu) {
            contextMenu = new ContextMenu(this);
            connect(contextMenu, &ContextMenu::showScriptSettings, this, &ViewerWidget::showScriptSettings);
        }
        contextMenu->setImageEntriesEnabled(isDisplaying());
        if(!contextMenu->isVisible())
            contextMenu->showAt(pos);
        else
            contextMenu->hide();
    }
}

void ViewerWidget::onFullscreenModeChanged(bool mode) {
    imageViewer->onFullscreenModeChanged(mode);
    mIsFullscreen = mode;
}

void ViewerWidget::readSettings() {
    videoControls->onVideoMuted(!settings->playVideoSounds());
    if(settings->clickableEdges()) {
        imageViewer->viewport()->installEventFilter(this);
        videoPlayer->installEventFilter(this);
        clickZoneOverlay->show();
    } else {
        imageViewer->viewport()->removeEventFilter(this);
        videoPlayer->removeEventFilter(this);
        imageViewer->enableDrags();
        clickZoneOverlay->hide();
    }
}

void ViewerWidget::setLoopPlayback(bool mode) {
    imageViewer->setLoopPlayback(mode);
    videoPlayer->setLoopPlayback(mode);
}

void ViewerWidget::hideContextMenu() {
    if(contextMenu)
        contextMenu->hide();
}

void ViewerWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    hideContextMenu();
}

// block native tab-switching so we can use it in shortcuts
bool ViewerWidget::focusNextPrevChild(bool mode) {
    return false;
}

void ViewerWidget::keyPressEvent(QKeyEvent *event) {
    event->accept();
    if(currentWidget == VIDEOPLAYER && event->key() == Qt::Key_Space) {
        videoPlayer->pauseResume();
        return;
    }
    if(currentWidget == IMAGEVIEWER && imageViewer->isDisplaying()) {
        // switch to fitWidth via up arrow
        if(ShortcutBuilder::fromEvent(event) == "Up" && !actionManager->actionForShortcut("Up").isEmpty()) {
            if(imageViewer->fitMode() == FIT_WINDOW && imageViewer->scaledImageFits()) {
                imageViewer->setFitWidth();
                return;
            }
        }
    }
    actionManager->processEvent(event);
}

void ViewerWidget::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    videoControls->hide();
}
