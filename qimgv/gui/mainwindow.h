#pragma once

#include <QApplication>
#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QMimeData>
#include <QImageWriter>
#include <QWindow>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <QDesktopWidget>
#endif

#include "gui/customwidgets/floatingwidgetcontainer.h"
#include "gui/viewers/viewerwidget.h"
#include "gui/overlays/controlsoverlay.h"
#include "gui/overlays/fullscreeninfooverlayproxy.h"
#include "gui/overlays/floatingmessageproxy.h"
#include "gui/overlays/saveconfirmoverlay.h"
#include "gui/panels/mainpanel/thumbnailstrip.h"
#include "gui/panels/sidepanel/sidepanel.h"
#include "gui/panels/croppanel/croppanel.h"
#include "gui/overlays/cropoverlay.h"
#include "gui/overlays/copyoverlay.h"
#include "gui/overlays/changelogwindow.h"
#include "gui/overlays/imageinfooverlayproxy.h"
#include "gui/overlays/renameoverlay.h"
#include "gui/dialogs/resizedialog.h"
#include "gui/centralwidget.h"
#include "gui/dialogs/filereplacedialog.h"
#include "components/actionmanager/actionmanager.h"
#include "settings.h"
#include "gui/dialogs/settingsdialog.h"
#include "gui/viewers/documentwidget.h"
#include "gui/folderview/folderviewproxy.h"
#include "gui/panels/infobar/infobarproxy.h"

#ifdef USE_KDE_BLUR
#include <KWindowEffects>
#endif

struct CurrentInfo {
    int index = 0;
    int fileCount = 0;
    QString fileName;
    QString filePath;
    QString directoryName;
    QString directoryPath;
    QSize imageSize;
    qint64 fileSize = 0;
    bool slideshow = false;
    bool shuffle = false;
    bool edited = false;
};

enum ActiveSidePanel {
    SIDEPANEL_CROP,
    SIDEPANEL_NONE
};

class MW : public FloatingWidgetContainer
{
    Q_OBJECT
public:
    explicit MW(QWidget *parent = nullptr);
    bool isCropPanelActive();
    void onScalingFinished(const QPixmap& scaled);
    void showImage(std::unique_ptr<QPixmap> pixmap);
    void showAnimation(const std::shared_ptr<QMovie>& movie);
    void showVideo(QString&& file);

    void setCurrentInfo(int fileIndex, int fileCount, const QString& filePath, const QString& fileName, QSize imageSize, qint64 fileSize, bool slideshow, bool shuffle, bool edited);
    std::shared_ptr<FolderViewProxy> getFolderView();
    std::shared_ptr<ThumbnailStripProxy> getThumbnailPanel();

    ViewMode currentViewMode();

    bool showConfirmation(const QString& title, const QString& msg);
    DialogResult fileReplaceDialog(QString source, QString target, FileReplaceMode mode, bool multiple);

private:
    std::shared_ptr<ViewerWidget> viewerWidget;            // 默认 nullptr
    QHBoxLayout layout;                                     // 默认构造
    QTimer windowGeometryChangeTimer;                       // 默认构造
    int currentDisplay = 0;
    bool showInfoBarFullscreen = false;
    bool showInfoBarWindowed = false;
    bool maximized = false;
    std::shared_ptr<DocumentWidget> docWidget;              // 默认 nullptr
    std::shared_ptr<FolderViewProxy> folderView;            // 默认 nullptr
    std::shared_ptr<CentralWidget> centralWidget;           // 默认 nullptr
    ActiveSidePanel activeSidePanel = SIDEPANEL_NONE;
    SidePanel *sidePanel = nullptr;
    CopyOverlay *copyOverlay = nullptr;
    SaveConfirmOverlay *saveOverlay = nullptr;
    RenameOverlay *renameOverlay = nullptr;
    FloatingMessageProxy *floatingMessage = nullptr;
    CropPanel *cropPanel = nullptr;
    CropOverlay *cropOverlay = nullptr;
    ChangelogWindow *changelogWindow = nullptr;
    ImageInfoOverlayProxy *imageInfoOverlay = nullptr;
    ControlsOverlay *controlsOverlay = nullptr;
    FullscreenInfoOverlayProxy *infoBarFullscreen = nullptr;
    std::shared_ptr<InfoBarProxy> infoBarWindowed;          // 默认 nullptr
    CurrentInfo info{};                                      // 默认构造（各字段已初始化）
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QDesktopWidget desktopWidget;
#endif

    QString cachedWindowTitle;
    QString cachedInfoText;
    QString cachedSizeText;
    bool fullUiInitialized = false;                         // 延迟初始化标志

    void saveWindowGeometry();
    void restoreWindowGeometry();
    void saveCurrentDisplay();
    void setupUi();

    void applyWindowedBackground();
    void applyFullscreenBackground();
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void setupCropPanel();
    void setupCopyOverlay();
    void setupSaveOverlay();
    void setupRenameOverlay();
    void preShowResize(QSize sz);
    void setInteractionEnabled(bool mode);

    // 辅助方法
    QString calculateWindowTitle();
    void calculateInfoBarContent(QString& infoText, QString& sizeText);

private slots:
    void updateCurrentDisplay();
    void readSettings();
    void adaptToWindowState();
    void onWindowGeometryChanged();
    void onInfoUpdated();
    void showScriptSettings();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
signals:
    void opened(QString);
    void fullscreenStateChanged(bool);
    void copyRequested(QString);
    void moveRequested(QString);
    void copyUrlsRequested(QList<QString>, QString);
    void moveUrlsRequested(QList<QString>, QString);
    void showFoldersChanged(bool);
    void resizeRequested(QSize);
    void renameRequested(QString);
    void cropRequested(QRect);
    void cropAndSaveRequested(QRect);
    void discardEditsRequested();
    void saveAsClicked();
    void saveRequested();
    void saveAsRequested(QString);
    void sortingSelected(SortingMode);

    // viewerWidget
    void scalingRequested(QSize, ScalingFilter);
    void zoomIn();
    void zoomOut();
    void zoomInCursor();
    void zoomOutCursor();
    void scrollUp();
    void scrollDown();
    void scrollLeft();
    void scrollRight();
    void pauseVideo();
    void stopPlayback();
    void seekVideoForward();
    void seekVideoBackward();
    void frameStep();
    void frameStepBack();
    void toggleMute();
    void volumeUp();
    void volumeDown();
    void toggleTransparencyGrid();
    void droppedIn(const QMimeData*, QObject*);
    void draggedOut();
    void setLoopPlayback(bool);
    void playbackFinished();

public slots:
    void setupFullUi();
    void showDefault();
    void showCropPanel();
    void hideCropPanel();
    void toggleFolderView();
    void enableFolderView();
    void enableDocumentView();
    void showOpenDialog(const QString& path);
    void showSaveDialog(const QString& filePath);
    QString getSaveFileName(const QString& fileName);
    void showResizeDialog(QSize initialSize);
    void showSettings();
    void triggerFullScreen();
    void showMessageDirectory(QString dirName);
    void showMessageDirectoryEnd();
    void showMessageDirectoryStart();
    void showMessageFitWindow();
    void showMessageFitWidth();
    void showMessageFitOriginal();
    void showFullScreen();
    void showWindowed();
    void triggerCopyOverlay();
    void showMessage(QString text);
    void showMessage(QString text, int duration);
    void showMessageSuccess(QString text);
    void showWarning(QString text);
    void showError(QString text);
    void triggerMoveOverlay();
    void closeFullScreenOrExit();
    void close();
    void triggerCropPanel();
    void updateCropPanelData();
    void showSaveOverlay();
    void hideSaveOverlay();
    void showChangelogWindow();
    void showChangelogWindow(QString text);
    void fitWindow();
    void fitWidth();
    void fitOriginal();
    void fitWindowStretch();
    void switchFitMode();
    void closeImage();
    void showContextMenu();
    void onSortingChanged(SortingMode);
    void toggleImageInfoOverlay();
    void toggleRenameOverlay(const QString& currentName);
    void setFilterNearest();
    void setFilterBilinear();
    void setFilter(ScalingFilter filter);
    void toggleScalingFilter();
    void setDirectoryPath(const QString& path);
    void toggleLockZoom();
    void toggleLockView();
    void toggleFullscreenInfoBar();
};