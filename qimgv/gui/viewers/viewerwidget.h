#pragma once

#include "gui/customwidgets/floatingwidgetcontainer.h"
#include <QVBoxLayout>
#include "gui/viewers/imageviewerv2.h"
#include "gui/viewers/videoplayerinitproxy.h"
#include "gui/overlays/videocontrolsproxy.h"
#include "gui/overlays/zoomindicatoroverlayproxy.h"
#include "gui/overlays/clickzoneoverlay.h"
#include "gui/contextmenu.h"
#include <QPointer>

class ImageAnimated;

enum CurrentWidget {
    IMAGEVIEWER,
    VIDEOPLAYER,
    UNSET
};

class ViewerWidget : public FloatingWidgetContainer
{
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget *parent = nullptr);
    QRect imageRect();
    float currentScale();
    QSize sourceSize();

    void setInteractionEnabled(bool mode);
    bool interactionEnabled();

    bool showImage(const QPixmap& pixmap);
    bool showAnimation(const std::shared_ptr<ImageAnimated>& image);
    void onScalingFinished(const QPixmap& scaled);
    bool isDisplaying();
    bool lockZoomEnabled();
    bool lockViewEnabled();

private:
    QVBoxLayout *layout;
    QPointer<ImageViewerV2> imageViewer;
    QPointer<VideoPlayerInitProxy> videoPlayer;
    QPointer<ContextMenu> contextMenu;
    VideoControlsProxyWrapper *videoControls;
    ZoomIndicatorOverlayProxy *zoomIndicator;
    ClickZoneOverlay *clickZoneOverlay;

    void enableImageViewer();
    void enableVideoPlayer();

    CurrentWidget currentWidget;
    bool mInteractionEnabled, mWaylandCursorWorkaround;
    QTimer cursorTimer;
    const int CURSOR_HIDE_TIMEOUT_MS = 1000;
    bool mIsFullscreen;

    void disableImageViewer();
    void disableVideoPlayer();

    QRect videoControlsArea();
    QRect mVideoControlsArea;
    bool mVideoControlsAreaValid = false;

    bool eventFilter(QObject *object, QEvent *event) override;

private slots:
    void onScaleChanged(qreal, bool);
    void onVideoPlaybackFinished();
    void onAnimationPlaybackFinished();

signals:
    void scalingRequested(QSize);
    void zoomIn();
    void zoomOut();
    void zoomInCursor();
    void zoomOutCursor();
    void scrollUp();
    void scrollDown();
    void scrollLeft();
    void scrollRight();
    void fitWindow();
    void fitWidth();
    void fitOriginal();
    void fitWindowStretch();
    void toggleTransparencyGrid();
    void draggedOut();
    void playbackFinished();
    void toggleLockZoom();
    void toggleLockView();
    void showScriptSettings();

public slots:
    bool showVideo(QString file);
    void stopPlayback();
    void setFitMode(ImageFitMode mode);
    ImageFitMode fitMode();
    void closeImage();
    void hideCursor();
    void showCursor();
    void hideCursorTimed(bool restartTimer);

    // video control
    void pauseResumePlayback();
    void seek(int pos);
    void seekRelative(int pos);
    void seekBackward();
    void seekForward();
    void frameStep();
    void frameStepBack();
    void toggleMute();
    void volumeUp();
    void volumeDown();

    void startPlayback();
    void resumeAnimation();
    void showContextMenu();
    void hideContextMenu();
    void showContextMenu(QPoint pos);
    void onFullscreenModeChanged(bool);
    void readSettings();
    void setLoopPlayback(bool mode);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool focusNextPrevChild(bool mode) override;
    void resizeEvent(QResizeEvent *event) override;
};
