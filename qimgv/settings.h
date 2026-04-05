#pragma once

#include <QObject>
#include <QSettings>
#include <QApplication>
#include <QStandardPaths>
#include <QDebug>
#include <QImageReader>
#include <QStringList>
#include <QColor>
#include <QPalette>
#include <QDir>
#include <QKeySequence>
#include <QMap>
#include <QFont>
#include <QFontMetrics>
#include <QVersionNumber>
#include <QThread>
#include "utils/script.h"
#include "themestore.h"

enum SortingMode {
    SORT_NAME,
    SORT_NAME_DESC,
    SORT_SIZE,
    SORT_SIZE_DESC,
    SORT_TIME,
    SORT_TIME_DESC
};

enum ImageFitMode {
    FIT_WINDOW,
    FIT_WIDTH,
    FIT_ORIGINAL,
    FIT_WINDOW_STRETCH,
    FIT_FREE
};

enum PanelPosition {
    PANEL_TOP,
    PANEL_BOTTOM,
    PANEL_LEFT,
    PANEL_RIGHT
};

enum ScalingFilter {
    QI_FILTER_NEAREST,
    QI_FILTER_BILINEAR,
    QI_FILTER_CV_BILINEAR_SHARPEN,
    QI_FILTER_CV_CUBIC,
    QI_FILTER_CV_CUBIC_SHARPEN
};

enum ZoomIndicatorMode {
    INDICATOR_DISABLED,
    INDICATOR_ENABLED,
    INDICATOR_AUTO
};

enum DefaultCropAction {
    ACTION_CROP,
    ACTION_CROP_SAVE
};

enum ImageFocusPoint {
    FOCUS_TOP,
    FOCUS_CENTER,
    FOCUS_CURSOR
};

enum ImageScrolling {
    SCROLL_NONE,
    SCROLL_BY_TRACKPAD,
    SCROLL_BY_TRACKPAD_AND_WHEEL
};

enum FolderEndAction {
    FOLDER_END_NO_ACTION,
    FOLDER_END_LOOP,
    FOLDER_END_GOTO_ADJACENT
};

enum ThumbPanelStyle {
    TH_PANEL_SIMPLE,
    TH_PANEL_EXTENDED
};

class Settings : public QObject
{
    Q_OBJECT
public:
    static Settings* getInstance();
    ~Settings();
    QStringList supportedMimeTypes();
    QList<QByteArray> supportedFormats();
    QString supportedFormatsFilter();
    QString supportedFormatsRegex();
    int panelPreviewsSize();
    void setPanelPreviewsSize(int size);
    bool usePreloader();
    void setUsePreloader(bool mode);
    bool fullscreenMode();
    void setFullscreenMode(bool mode);
    ImageFitMode imageFitMode();
    void setImageFitMode(ImageFitMode mode);
    QRect windowGeometry();
    void setWindowGeometry(QRect geometry);
    bool playVideoSounds();
    void setPlayVideoSounds(bool mode);
    void setVolume(int vol);
    int volume();
    QString thumbnailCacheDir();
    QString mpvBinary();
    void setMpvBinary(const QString &path);
    PanelPosition panelPosition();
    void setPanelPosition(PanelPosition);
    bool loopSlideshow();
    void setLoopSlideshow(bool mode);
    void readShortcuts(QMap<QString, QString> &shortcuts);
    void saveShortcuts(const QMap<QString, QString> &shortcuts);
    bool panelEnabled();
    void setPanelEnabled(bool mode);
    int lastDisplay();
    void setLastDisplay(int display);
    bool squareThumbnails();
    void setSquareThumbnails(bool mode);
    bool transparencyGrid();
    void setTransparencyGrid(bool mode);
    bool enableSmoothScroll();
    void setEnableSmoothScroll(bool mode);
    bool useThumbnailCache();
    void setUseThumbnailCache(bool mode);
    QStringList savedPaths();
    void setSavedPaths(const QStringList &paths);
    QString tmpDir();
    int thumbnailerThreadCount();
    void setThumbnailerThreadCount(int count);
    bool smoothUpscaling();
    void setSmoothUpscaling(bool mode);
    void setExpandImage(bool mode);
    bool expandImage();
    ScalingFilter scalingFilter();
    void setScalingFilter(ScalingFilter mode);
    bool smoothAnimatedImages();
    void setSmoothAnimatedImages(bool mode);
    bool panelFullscreenOnly();
    void setPanelFullscreenOnly(bool mode);
    QVersionNumber lastVersion();
    void setLastVersion(const QVersionNumber &ver);
    void setShowChangelogs(bool mode);
    bool showChangelogs();
    qreal backgroundOpacity();
    void setBackgroundOpacity(qreal value);
    bool blurBackground();
    void setBlurBackground(bool mode);
    void setSortingMode(SortingMode mode);
    SortingMode sortingMode();    
    void readScripts(QHash<QString, Script> &scripts);
    void saveScripts(const QHash<QString, Script> &scripts);
    int folderViewIconSize();
    void setFolderViewIconSize(int value);

    bool firstRun();
    void setFirstRun(bool mode);

    void sync();
    bool cursorAutohide();
    void setCursorAutohide(bool mode);

    bool infoBarFullscreen();
    void setInfoBarFullscreen(bool mode);
    bool infoBarWindowed();
    void setInfoBarWindowed(bool mode);

    bool windowTitleExtendedInfo();
    void setWindowTitleExtendedInfo(bool mode);

    bool maximizedWindow();
    void setMaximizedWindow(bool mode);

    bool keepFitMode();
    void setKeepFitMode(bool mode);

    int expandLimit();
    void setExpandLimit(int value);

    float zoomStep();
    void setZoomStep(float value);
    int JPEGSaveQuality();
    void setJPEGSaveQuality(int value);
    void setZoomIndicatorMode(ZoomIndicatorMode mode);
    ZoomIndicatorMode zoomIndicatorMode();
    void setFocusPointIn1to1Mode(ImageFocusPoint mode);
    ImageFocusPoint focusPointIn1to1Mode();
    void setDefaultCropAction(DefaultCropAction mode);
    DefaultCropAction defaultCropAction();
    bool placesPanel();
    void setPlacesPanel(bool mode);

    QStringList bookmarks();
    void setBookmarks(const QStringList &paths);
    bool placesPanelBookmarksExpanded();
    void setPlacesPanelBookmarksExpanded(bool mode);
    bool placesPanelTreeExpanded();
    void setPlacesPanelTreeExpanded(bool mode);

    void setSlideshowInterval(int ms);
    int slideshowInterval();

    ImageScrolling imageScrolling();
    void setImageScrolling(ImageScrolling mode);

    int placesPanelWidth();
    void setPlacesPanelWidth(int width);

    FolderEndAction folderEndAction();
    void setFolderEndAction(FolderEndAction mode);

    const ColorScheme& colorScheme();
    void setColorScheme(ColorScheme scheme);
    void setColorTid(int tid);

    bool videoPlayback();
    void setVideoPlayback(bool mode);

    bool useSystemColorScheme();
    void setUseSystemColorScheme(bool mode);

    void loadStylesheet();

    bool showSaveOverlay();
    void setShowSaveOverlay(bool mode);
    bool confirmDelete();
    void setConfirmDelete(bool mode);
    bool confirmTrash();
    void setConfirmTrash(bool mode);

    const QMultiMap<QByteArray, QByteArray> videoFormats() const;

    bool printLandscape();
    void setPrintLandscape(bool mode);
    bool printPdfDefault();
    void setPrintPdfDefault(bool mode);
    bool printColor();
    void setPrintColor(bool mode);
    bool printFitToPage();
    void setPrintFitToPage(bool mode);
    QString lastPrinter();
    void setLastPrinter(const QString &name);
    bool unloadThumbs();
    void setUnloadThumbs(bool mode);
    ThumbPanelStyle thumbPanelStyle();
    void setThumbPanelStyle(ThumbPanelStyle mode);

    bool jxlAnimation();
    void setJxlAnimation(bool mode);
    bool absoluteZoomStep();
    void setAbsoluteZoomStep(bool mode);
    bool autoResizeWindow();
    void setAutoResizeWindow(bool mode);
    int autoResizeLimit();
    void setAutoResizeLimit(int percent);

    bool panelPinned();
    void setPanelPinned(bool mode);
    int memoryAllocationLimit();
    void setMemoryAllocationLimit(int limitMB);
    bool panelCenterSelection();
    void setPanelCenterSelection(bool mode);
    QString language();
    void setLanguage(const QString &lang);

    QString defaultZoomLevels();
    QString zoomLevels();
    void setZoomLevels(const QString &levels);
    bool useFixedZoomLevels();
    void setUseFixedZoomLevels(bool mode);
    bool unlockMinZoom();
    void setUnlockMinZoom(bool mode);
    bool sortFolders();
    void setSortFolders(bool mode);
    bool trackpadDetection();
    void setTrackpadDetection(bool mode);

    bool clickableEdges();
    void setClickableEdges(bool mode);
    bool clickableEdgesVisible();
    void setClickableEdgesVisible(bool mode);

    float mouseScrollingSpeed();
    void setMouseScrollingSpeed(float value);

    bool showHiddenFiles();
    void setShowHiddenFiles(bool mode);

private:
    explicit Settings(QObject *parent = nullptr);
    QSettings *settingsConf = nullptr;
    QSettings *stateConf = nullptr;
    QSettings *themeConf = nullptr;
    QDir *mTmpDir = nullptr;
    QDir *mThumbCacheDir = nullptr;
    QDir *mConfDir = nullptr;
    ColorScheme mColorScheme;
    QMultiMap<QByteArray, QByteArray> mVideoFormatsMap; // [mimetype, format]
    
    // 缓存支持的格式，避免重复计算
    mutable QList<QByteArray> mCachedSupportedFormats;
    mutable QStringList mCachedSupportedMimeTypes;
    mutable bool mFormatsCacheValid;
    mutable bool mMimeTypesCacheValid;
    
    // 缓存格式过滤器和正则表达式，避免重复构建
    mutable QString mCachedFormatsFilter;
    mutable bool mFormatsFilterCacheValid;
    mutable QString mCachedFormatsRegex;
    mutable bool mFormatsRegexCacheValid;
    
    // 缓存loadStylesheet()结果
    mutable QString mCachedStylesheet;
    mutable bool mStylesheetCacheValid;
    
    // 缓存浮点设置值
    mutable float mCachedZoomStep;
    mutable bool mZoomStepCacheValid;
    mutable float mCachedMouseScrollingSpeed;
    mutable bool mMouseScrollingSpeedCacheValid;
    
    // 缓存枚举设置值
    mutable ScalingFilter mCachedScalingFilter;
    mutable bool mScalingFilterCacheValid;
    mutable QRect mCachedWindowGeometry;
    mutable bool mWindowGeometryCacheValid;
    mutable ImageFitMode mCachedImageFitMode;
    mutable bool mImageFitModeCacheValid;
    
    // 缓存整数设置值
    mutable int mCachedPanelPreviewsSize;
    mutable bool mPanelPreviewsSizeCacheValid;
    mutable int mCachedJPEGSaveQuality;
    mutable bool mJPEGSaveQualityCacheValid;
    
    // 缓存字符串列表
    mutable QStringList mCachedSavedPaths;
    mutable bool mSavedPathsCacheValid;
    mutable QStringList mCachedBookmarks;
    mutable bool mBookmarksCacheValid;
    
    // 缓存快捷键和脚本
    mutable QMap<QString, QString> mCachedShortcuts;
    mutable bool mShortcutsCacheValid;
    mutable QHash<QString, Script> mCachedScripts;
    mutable bool mScriptsCacheValid;
    mutable PanelPosition mCachedPanelPosition;
    mutable bool mPanelPositionCacheValid;
    mutable SortingMode mCachedSortingMode;
    mutable bool mSortingModeCacheValid;
    mutable ZoomIndicatorMode mCachedZoomIndicatorMode;
    mutable bool mZoomIndicatorModeCacheValid;
    mutable DefaultCropAction mCachedDefaultCropAction;
    mutable bool mDefaultCropActionCacheValid;
    mutable ImageFocusPoint mCachedFocusPointIn1to1Mode;
    mutable bool mFocusPointIn1to1ModeCacheValid;
    mutable ImageScrolling mCachedImageScrolling;
    mutable bool mImageScrollingCacheValid;
    mutable FolderEndAction mCachedFolderEndAction;
    mutable bool mFolderEndActionCacheValid;
    mutable ThumbPanelStyle mCachedThumbPanelStyle;
    mutable bool mThumbPanelStyleCacheValid;
    
    void loadTheme();
    void saveTheme();
    void createColorVariants();

    void setupCache();
    void fillVideoFormats();

signals:
    void settingsChanged();

public slots:
    void sendChangeNotification();

};

extern Settings *settings;
