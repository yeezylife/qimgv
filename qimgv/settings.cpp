#include "settings.h"
#include <QHash>

Settings *settings = nullptr;

Settings::Settings(QObject *parent) : QObject(parent) {
#if defined(__linux__) || defined(__FreeBSD__)
    // config files
    QSettings::setDefaultFormat(QSettings::NativeFormat);
    settingsConf = std::make_unique<QSettings>();
    stateConf = std::make_unique<QSettings>(QCoreApplication::organizationName(), "savedState");
    themeConf = std::make_unique<QSettings>(QCoreApplication::organizationName(), "theme");
#else
    mConfDir = std::make_unique<QDir>(QApplication::applicationDirPath() + "/conf");
    mConfDir->mkpath(QApplication::applicationDirPath() + "/conf");
    settingsConf = std::make_unique<QSettings>(mConfDir->absolutePath() + "/" + qApp->applicationName() + ".ini", QSettings::IniFormat);
    stateConf = std::make_unique<QSettings>(mConfDir->absolutePath() + "/savedState.ini", QSettings::IniFormat);
    themeConf = std::make_unique<QSettings>(mConfDir->absolutePath() + "/theme.ini", QSettings::IniFormat);
#endif
    fillVideoFormats();
    mFormatsCacheValid = false;
    mMimeTypesCacheValid = false;
    mFormatsFilterCacheValid = false;
    mFormatsRegexCacheValid = false;
    
    // 初始化缓存标志
    mStylesheetCacheValid = false;
    mZoomStepCacheValid = false;
    mMouseScrollingSpeedCacheValid = false;
    mScalingFilterCacheValid = false;
    mWindowGeometryCacheValid = false;
    mImageFitModeCacheValid = false;
    mPanelPreviewsSizeCacheValid = false;
    mJPEGSaveQualityCacheValid = false;
    mSavedPathsCacheValid = false;
    mBookmarksCacheValid = false;
    mShortcutsCacheValid = false;
    mScriptsCacheValid = false;
    mPanelPositionCacheValid = false;
    mSortingModeCacheValid = false;
    mZoomIndicatorModeCacheValid = false;
    mDefaultCropActionCacheValid = false;
    mFocusPointIn1to1ModeCacheValid = false;
    mImageScrollingCacheValid = false;
    mFolderEndActionCacheValid = false;
}
//------------------------------------------------------------------------------
Settings::~Settings() {
    saveTheme();
}
//------------------------------------------------------------------------------
Settings *Settings::getInstance() {
    if(!settings) {
        settings = new Settings();
        settings->setupCache();
        settings->loadTheme();
    }
    return settings;
}
//------------------------------------------------------------------------------
void Settings::setupCache() {
#if defined(__linux__) ||  defined(__FreeBSD__)
    QString genericCacheLocation = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    if(genericCacheLocation.isEmpty())
        genericCacheLocation = QDir::homePath() + "/.cache";
    genericCacheLocation.append("/" + QApplication::applicationName());
    QString cacheLocation = settingsConf->value("cacheDir", genericCacheLocation).toString();
    mTmpDir = std::make_unique<QDir>(cacheLocation);
    mTmpDir->mkpath(mTmpDir->absolutePath());
    QFileInfo dirTest(mTmpDir->absolutePath());
    if(!dirTest.isDir() || !dirTest.isWritable() || !dirTest.exists()) {
        // fallback
        qDebug() << "Error: cache dir is not writable" << mTmpDir->absolutePath();
        qDebug() << "Trying to use" << genericCacheLocation << "instead";
        mTmpDir->setPath(genericCacheLocation);
        mTmpDir->mkpath(mTmpDir->absolutePath());
    }
#else
    mTmpDir = std::make_unique<QDir>(QApplication::applicationDirPath() + "/cache");
    mTmpDir->mkpath(mTmpDir->absolutePath());
#endif
}
//------------------------------------------------------------------------------
void Settings::sync() {
    settingsConf->sync();
    stateConf->sync();
}
//------------------------------------------------------------------------------
QString Settings::tmpDir() {
    return mTmpDir->path() + "/";
}
//------------------------------------------------------------------------------
// this here is temporarily, will be moved to some sort of theme manager class
void Settings::loadStylesheet() {
    // 检查缓存是否有效
    if (mStylesheetCacheValid) {
        qApp->setStyleSheet(mCachedStylesheet);
        return;
    }
    
    // stylesheet template file
    QFile file(":/res/styles/style-template.qss");
    if(file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());

        // --- color scheme ---------------------------------------------
    const auto& colors = colorScheme();
        // tint color for system windows
        QPalette p;
        QColor sys_text = p.text().color();
        QColor sys_window = p.window().color();
        QColor sys_window_tinted, sys_window_tinted_lc, sys_window_tinted_lc2, sys_window_tinted_hc, sys_window_tinted_hc2;
        if(sys_window.valueF() <= 0.45f) {
            // dark system theme
            sys_window_tinted_lc2.setHsv(sys_window.hue(), sys_window.saturation(), sys_window.value() + 6);
            sys_window_tinted_lc.setHsv(sys_window.hue(),  sys_window.saturation(), sys_window.value() + 14);
            sys_window_tinted.setHsv(sys_window.hue(),     sys_window.saturation(), sys_window.value() + 20);
            sys_window_tinted_hc.setHsv(sys_window.hue(),  sys_window.saturation(), sys_window.value() + 35);
            sys_window_tinted_hc2.setHsv(sys_window.hue(), sys_window.saturation(), sys_window.value() + 50);
        } else {
            // light system theme
            sys_window_tinted_lc2.setHsv(sys_window.hue(), sys_window.saturation(), sys_window.value() - 6);
            sys_window_tinted_lc.setHsv(sys_window.hue(),  sys_window.saturation(), sys_window.value() - 14);
            sys_window_tinted.setHsv(sys_window.hue(),     sys_window.saturation(), sys_window.value() - 20);
            sys_window_tinted_hc.setHsv(sys_window.hue(),  sys_window.saturation(), sys_window.value() - 35);
            sys_window_tinted_hc2.setHsv(sys_window.hue(), sys_window.saturation(), sys_window.value() - 50);
        }

        // --- widget sizes ---------------------------------------------
        auto fnt = QGuiApplication::font();
        QFontMetrics fm(fnt);
        // todo: use precise values for ~9-11 point sizes
        int font_small = qMax(static_cast<int>(std::lround(fnt.pointSize() * 0.9)), 8);
        int font_large = static_cast<int>(std::lround(fnt.pointSize() * 1.8));
        int text_height = fm.height();
        int text_padding = static_cast<int>(std::lround(text_height * 0.10));
        int text_padding_large = static_cast<int>(std::lround(text_height * 0.25));

        // folderview top panel item sizes
        int top_panel_v_margin = 4;
        // ensure at least 4px so its not too thin
        int top_panel_text_padding = qMax(text_padding, 4);
        // scale with font, 38px base size
        int top_panel_height = qMax((text_height + top_panel_text_padding * 2 + top_panel_v_margin * 2), 38);

        // overlay headers
        // 32px base size
        int overlay_header_size = qMax(text_height + text_padding * 2, 30);

        // todo
        int button_height = text_height + text_padding_large * 2;

        // pseudo-dpi to scale some widget widths
        int text_height_base = 22;
        qreal pDpr = qMax( static_cast<qreal>(text_height) / text_height_base, 1.0);
        int context_menu_width = static_cast<int>(212 * pDpr);
        int context_menu_button_height = static_cast<int>(32 * pDpr);
        int rename_overlay_width = static_cast<int>(380 * pDpr);

        //qDebug()<< "dpr=" << qApp->devicePixelRatio() << "pDpr=" << pDpr;

        // --- write variables into stylesheet --------------------------
        styleSheet.replace("%font_small%", QString::number(font_small)+"pt");
        styleSheet.replace("%font_large%", QString::number(font_large)+"pt");
        styleSheet.replace("%button_height%", QString::number(button_height)+"px");
        styleSheet.replace("%top_panel_height%", QString::number(top_panel_height)+"px");
        styleSheet.replace("%overlay_header_size%", QString::number(overlay_header_size)+"px");
        styleSheet.replace("%context_menu_width%", QString::number(context_menu_width)+"px");
        styleSheet.replace("%context_menu_button_height%", QString::number(context_menu_button_height)+"px");
        styleSheet.replace("%rename_overlay_width%", QString::number(rename_overlay_width)+"px");

        styleSheet.replace("%icontheme%",  "light");
        // Qt::Popup can't do transparency under windows, use square window
#ifdef _WIN32
        styleSheet.replace("%contextmenu_border_radius%",  "0px");
#else
        styleSheet.replace("%contextmenu_border_radius%",  "3px");
#endif
        styleSheet.replace("%sys_window%",    sys_window.name());
        styleSheet.replace("%sys_window_tinted%",    sys_window_tinted.name());
        styleSheet.replace("%sys_window_tinted_lc%", sys_window_tinted_lc.name());
        styleSheet.replace("%sys_window_tinted_lc2%", sys_window_tinted_lc2.name());
        styleSheet.replace("%sys_window_tinted_hc%", sys_window_tinted_hc.name());
        styleSheet.replace("%sys_window_tinted_hc2%", sys_window_tinted_hc2.name());
        styleSheet.replace("%sys_text_secondary_rgba%", "rgba(" + QString::number(sys_text.red())   + ","
                                                      + QString::number(sys_text.green()) + ","
                                                      + QString::number(sys_text.blue())  + ",50%)");

        styleSheet.replace("%button%",               colors.button.name());
        styleSheet.replace("%button_hover%",         colors.button_hover.name());
        styleSheet.replace("%button_pressed%",       colors.button_pressed.name());
        styleSheet.replace("%panel_button%",         colors.panel_button.name());
        styleSheet.replace("%panel_button_hover%",   colors.panel_button_hover.name());
        styleSheet.replace("%panel_button_pressed%", colors.panel_button_pressed.name());
        styleSheet.replace("%widget%",               colors.widget.name());
        styleSheet.replace("%widget_border%",        colors.widget_border.name());
        styleSheet.replace("%folderview%",           colors.folderview.name());
        styleSheet.replace("%folderview_topbar%",    colors.folderview_topbar.name());
        styleSheet.replace("%folderview_hc%",        colors.folderview_hc.name());
        styleSheet.replace("%folderview_hc2%",       colors.folderview_hc2.name());
        styleSheet.replace("%accent%",               colors.accent.name());
        styleSheet.replace("%input_field_focus%",    colors.input_field_focus.name());
        styleSheet.replace("%overlay%",              colors.overlay.name());
        styleSheet.replace("%icons%",                colors.icons.name());
        styleSheet.replace("%text_hc2%",             colors.text_hc2.name());
        styleSheet.replace("%text_hc%",              colors.text_hc.name());
        styleSheet.replace("%text%",                 colors.text.name());
        styleSheet.replace("%overlay_text%",         colors.overlay_text.name());
        styleSheet.replace("%text_lc%",              colors.text_lc.name());
        styleSheet.replace("%text_lc2%",             colors.text_lc2.name());
        styleSheet.replace("%scrollbar%",            colors.scrollbar.name());
        styleSheet.replace("%scrollbar_hover%",      colors.scrollbar_hover.name());
        styleSheet.replace("%folderview_button_hover%",   colors.folderview_button_hover.name());
        styleSheet.replace("%folderview_button_pressed%", colors.folderview_button_pressed.name());
        styleSheet.replace("%text_secondary_rgba%",  "rgba(" + QString::number(colors.text.red())   + ","
                                                             + QString::number(colors.text.green()) + ","
                                                             + QString::number(colors.text.blue())  + ",62%)");
        styleSheet.replace("%accent_hover_rgba%",    "rgba(" + QString::number(colors.accent.red())   + ","
                                                             + QString::number(colors.accent.green()) + ","
                                                             + QString::number(colors.accent.blue())  + ",65%)");
        styleSheet.replace("%overlay_rgba%",         "rgba(" + QString::number(colors.overlay.red())   + ","
                                                             + QString::number(colors.overlay.green()) + ","
                                                             + QString::number(colors.overlay.blue())  + ",90%)");
        styleSheet.replace("%fv_backdrop_rgba%",     "rgba(" + QString::number(colors.folderview_hc2.red())   + ","
                                                             + QString::number(colors.folderview_hc2.green()) + ","
                                                             + QString::number(colors.folderview_hc2.blue())  + ",80%)");
        // do not show separator line if topbar color matches folderview
        if(colors.folderview != colors.folderview_topbar)
            styleSheet.replace("%topbar_border_rgba%", "rgba(0,0,0,14%)");
        else
            styleSheet.replace("%topbar_border_rgba%", colors.folderview.name());

        // --- apply -------------------------------------------------
        qApp->setStyleSheet(styleSheet);
        
        // 缓存样式表
        mCachedStylesheet = styleSheet;
        mStylesheetCacheValid = true;
    }
}
//------------------------------------------------------------------------------
void Settings::loadTheme() {
    if(useSystemColorScheme()) {
        setColorScheme(ThemeStore::colorScheme(ColorSchemes::COLORS_SYSTEM));
    } else {
        BaseColorScheme base;
        themeConf->beginGroup("Colors");
        base.background            = QColor(themeConf->value("background",            "#1a1a1a").toString());
        base.background_fullscreen = QColor(themeConf->value("background_fullscreen", "#1a1a1a").toString());
        base.text                  = QColor(themeConf->value("text",                  "#b6b6b6").toString());
        base.icons                 = QColor(themeConf->value("icons",                 "#a4a4a4").toString());
        base.widget                = QColor(themeConf->value("widget",                "#252525").toString());
        base.widget_border         = QColor(themeConf->value("widget_border",         "#2c2c2c").toString());
        base.accent                = QColor(themeConf->value("accent",                "#8c9b81").toString());
        base.folderview            = QColor(themeConf->value("folderview",            "#242424").toString());
        base.folderview_topbar     = QColor(themeConf->value("folderview_topbar",     "#383838").toString());
        base.scrollbar             = QColor(themeConf->value("scrollbar",             "#5a5a5a").toString());
        base.overlay_text          = QColor(themeConf->value("overlay_text",          "#d2d2d2").toString());
        base.overlay               = QColor(themeConf->value("overlay",               "#1a1a1a").toString());
        base.tid                   = themeConf->value("tid", "-1").toInt();
        themeConf->endGroup();
        setColorScheme(ColorScheme(base));
    }
}
void Settings::saveTheme() {
    if(useSystemColorScheme())
        return;
    themeConf->beginGroup("Colors");
    themeConf->setValue("background",            mColorScheme.background.name());
    themeConf->setValue("background_fullscreen", mColorScheme.background_fullscreen.name());
    themeConf->setValue("text",                  mColorScheme.text.name());
    themeConf->setValue("icons",                 mColorScheme.icons.name());
    themeConf->setValue("widget",                mColorScheme.widget.name());
    themeConf->setValue("widget_border",         mColorScheme.widget_border.name());
    themeConf->setValue("accent",                mColorScheme.accent.name());
    themeConf->setValue("folderview",            mColorScheme.folderview.name());
    themeConf->setValue("folderview_topbar",     mColorScheme.folderview_topbar.name());
    themeConf->setValue("scrollbar",             mColorScheme.scrollbar.name());
    themeConf->setValue("overlay_text",          mColorScheme.overlay_text.name());
    themeConf->setValue("overlay",               mColorScheme.overlay.name());
    themeConf->setValue("tid",                   mColorScheme.tid);
    themeConf->endGroup();
}
//------------------------------------------------------------------------------
const ColorScheme& Settings::colorScheme() {
    return mColorScheme;
}
//------------------------------------------------------------------------------
void Settings::setColorScheme(ColorScheme scheme) {
    mColorScheme = scheme;
    mStylesheetCacheValid = false;
    loadStylesheet();
}
//------------------------------------------------------------------------------
void Settings::setColorTid(int tid) {
    mColorScheme.tid = tid;
}
//------------------------------------------------------------------------------
void Settings::fillVideoFormats() {
    mVideoFormatsMap.insert("video/webm",       "webm");
    mVideoFormatsMap.insert("video/mp4",        "mp4");
    mVideoFormatsMap.insert("video/mp4",        "m4v");
    mVideoFormatsMap.insert("video/mpeg",       "mpg");
    mVideoFormatsMap.insert("video/mpeg",       "mpeg");
    mVideoFormatsMap.insert("video/x-matroska", "mkv");
    mVideoFormatsMap.insert("video/x-ms-wmv",   "wmv");
    mVideoFormatsMap.insert("video/x-msvideo",  "avi");
    mVideoFormatsMap.insert("video/quicktime",  "mov");
    mVideoFormatsMap.insert("video/x-flv",      "flv");
}
//------------------------------------------------------------------------------
QString Settings::mpvBinary() {
    QString mpvPath = settingsConf->value("mpvBinary", "").toString();
    if(!QFile::exists(mpvPath)) {
    #ifdef _WIN32
        mpvPath = QCoreApplication::applicationDirPath() + "/mpv.exe";
    #elif defined __linux__
        mpvPath = "/usr/bin/mpv";
    #elif defined __FreeBSD__
        mpvPath = "/usr/local/bin/mpv";
    #endif
        if(!QFile::exists(mpvPath))
            mpvPath = "";
    }
    return mpvPath;
}

void Settings::setMpvBinary(const QString &path) {
    if(QFile::exists(path)) {
        settingsConf->setValue("mpvBinary", path);
    }
}
//------------------------------------------------------------------------------
QList<QByteArray> Settings::supportedFormats() {
    if (!mFormatsCacheValid) {
        mCachedSupportedFormats = QImageReader::supportedImageFormats();
        mCachedSupportedFormats << "jfif";
        if(videoPlayback())
            mCachedSupportedFormats << mVideoFormatsMap.values();
        mCachedSupportedFormats.removeAll("pdf");
        mFormatsCacheValid = true;
    }
    return mCachedSupportedFormats;
}
//------------------------------------------------------------------------------
// (for open/save dialogs, as a single string)
// example:  "Images (*.jpg, *.png)"
QString Settings::supportedFormatsFilter() {
    if (!mFormatsFilterCacheValid) {
        auto formats = supportedFormats();
        QStringList formatList;
        formatList.reserve(formats.count());
        for(const auto &fmt : formats)
            formatList << "*." + QString(fmt);
        mCachedFormatsFilter = "Supported files (" + formatList.join(" ") + ")";
        mFormatsFilterCacheValid = true;
    }
    return mCachedFormatsFilter;
}
//------------------------------------------------------------------------------
QString Settings::supportedFormatsRegex() {
    if (!mFormatsRegexCacheValid) {
        auto formats = supportedFormats();
        QStringList formatList;
        formatList.reserve(formats.count());
        for(const auto &fmt : formats)
            formatList << QString(fmt);
        mCachedFormatsRegex = ".*\\.(" + formatList.join("|") + ")$";
        mFormatsRegexCacheValid = true;
    }
    return mCachedFormatsRegex;
}
//------------------------------------------------------------------------------
// returns list of mime types
QStringList Settings::supportedMimeTypes() {
    if (!mMimeTypesCacheValid) {
        mCachedSupportedMimeTypes.clear();
        QList<QByteArray> mimeTypes = QImageReader::supportedMimeTypes();
        if(videoPlayback())
            mimeTypes << mVideoFormatsMap.keys();
        for(int i = 0; i < mimeTypes.count(); i++) {
            mCachedSupportedMimeTypes << QString(mimeTypes.at(i));
        }
        mMimeTypesCacheValid = true;
    }
    return mCachedSupportedMimeTypes;
}
//------------------------------------------------------------------------------
bool Settings::videoPlayback() {
#ifdef USE_MPV
    return settingsConf->value("videoPlayback", true).toBool();
#else
    return false;
#endif
}

void Settings::setVideoPlayback(bool mode) {
    settingsConf->setValue("videoPlayback", mode);
    mFormatsCacheValid = false;
    mMimeTypesCacheValid = false;
    mFormatsFilterCacheValid = false;
    mFormatsRegexCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::useSystemColorScheme() {
    return settingsConf->value("useSystemColorScheme", false).toBool();
}

void Settings::setUseSystemColorScheme(bool mode) {
    settingsConf->setValue("useSystemColorScheme", mode);
}
//------------------------------------------------------------------------------
QVersionNumber Settings::lastVersion() {
    int vmajor = settingsConf->value("lastVerMajor", 0).toInt();
    int vminor = settingsConf->value("lastVerMinor", 0).toInt();
    int vmicro = settingsConf->value("lastVerMicro", 0).toInt();
    return QVersionNumber(vmajor, vminor, vmicro);
}

void Settings::setLastVersion(const QVersionNumber &ver) {
    settingsConf->setValue("lastVerMajor", ver.majorVersion());
    settingsConf->setValue("lastVerMinor", ver.minorVersion());
    settingsConf->setValue("lastVerMicro", ver.microVersion());
}
//------------------------------------------------------------------------------
void Settings::setShowChangelogs(bool mode) {
    settingsConf->setValue("showChangelogs", mode);
}

bool Settings::showChangelogs() {
    return settingsConf->value("showChangelogs", true).toBool();
}
//------------------------------------------------------------------------------
qreal Settings::backgroundOpacity() {
    bool ok = false;
    qreal value = settingsConf->value("backgroundOpacity", 1.0).toReal(&ok);
    if(!ok)
        return 0.0;
    if(value > 1.0)
        return 1.0;
    if(value < 0.0)
        return 0.0;
    return value;
}

void Settings::setBackgroundOpacity(qreal value) {
    if(value > 1.0)
        value = 1.0;
    else if(value < 0.0)
        value = 0.0;
    settingsConf->setValue("backgroundOpacity", value);
}
//------------------------------------------------------------------------------
bool Settings::blurBackground() {
#ifndef USE_KDE_BLUR
    return false;
#endif
    return settingsConf->value("blurBackground", true).toBool();
}

void Settings::setBlurBackground(bool mode) {
    settingsConf->setValue("blurBackground", mode);
}
//------------------------------------------------------------------------------
void Settings::setSortingMode(SortingMode mode) {
    if(mode >= 6)
        mode = SortingMode::SORT_NAME;
    settingsConf->setValue("sortingMode", mode);
    mSortingModeCacheValid = false;
}

SortingMode Settings::sortingMode() {
    if (mSortingModeCacheValid) {
        return mCachedSortingMode;
    }
    
    int mode = settingsConf->value("sortingMode", 0).toInt();
    if(mode < 0 || mode >= 6)
        mode = 0;
    
    mCachedSortingMode = static_cast<SortingMode>(mode);
    mSortingModeCacheValid = true;
    return mCachedSortingMode;
}
//------------------------------------------------------------------------------
bool Settings::playVideoSounds() {
    return settingsConf->value("playVideoSounds", false).toBool();
}

void Settings::setPlayVideoSounds(bool mode) {
    settingsConf->setValue("playVideoSounds", mode);
}
//------------------------------------------------------------------------------
void Settings::setVolume(int vol) {
    stateConf->setValue("volume", vol);
}

int Settings::volume() {
    return stateConf->value("volume", 100).toInt();
}
//------------------------------------------------------------------------------
const QMultiMap<QByteArray, QByteArray> Settings::videoFormats() const {
    return mVideoFormatsMap;
}
//------------------------------------------------------------------------------
int Settings::panelPreviewsSize() {
    if (mPanelPreviewsSizeCacheValid) {
        return mCachedPanelPreviewsSize;
    }
    
    bool ok = true;
    int size = settingsConf->value("panelPreviewsSize", 140).toInt(&ok);
    if(!ok)
        size = 140;
    size = qBound(100, size, 250);
    
    mCachedPanelPreviewsSize = size;
    mPanelPreviewsSizeCacheValid = true;
    return size;
}

void Settings::setPanelPreviewsSize(int size) {
    settingsConf->setValue("panelPreviewsSize", size);
    mPanelPreviewsSizeCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::usePreloader() {
    return settingsConf->value("usePreloader", true).toBool();
}

void Settings::setUsePreloader(bool mode) {
    settingsConf->setValue("usePreloader", mode);
}
//------------------------------------------------------------------------------
bool Settings::keepFitMode() {
    return settingsConf->value("keepFitMode", false).toBool();
}

void Settings::setKeepFitMode(bool mode) {
    settingsConf->setValue("keepFitMode", mode);
}
//------------------------------------------------------------------------------
bool Settings::fullscreenMode() {
    return settingsConf->value("openInFullscreen", false).toBool();
}

void Settings::setFullscreenMode(bool mode) {
    settingsConf->setValue("openInFullscreen", mode);
}
//------------------------------------------------------------------------------
bool Settings::maximizedWindow() {
    return stateConf->value("maximizedWindow", false).toBool();
}

void Settings::setMaximizedWindow(bool mode) {
    stateConf->setValue("maximizedWindow", mode);
}
//------------------------------------------------------------------------------
bool Settings::panelEnabled() {
    return settingsConf->value("panelEnabled", true).toBool();
}

void Settings::setPanelEnabled(bool mode) {
    settingsConf->setValue("panelEnabled", mode);
}
//------------------------------------------------------------------------------
bool Settings::panelFullscreenOnly() {
    return settingsConf->value("panelFullscreenOnly", true).toBool();
}

void Settings::setPanelFullscreenOnly(bool mode) {
    settingsConf->setValue("panelFullscreenOnly", mode);
}
//------------------------------------------------------------------------------
int Settings::lastDisplay() {
    return stateConf->value("lastDisplay", 0).toInt();
}

void Settings::setLastDisplay(int display) {
    stateConf->setValue("lastDisplay", display);
}
//------------------------------------------------------------------------------
PanelPosition Settings::panelPosition() {
    if (mPanelPositionCacheValid) {
        return mCachedPanelPosition;
    }
    
    static const QHash<QString, PanelPosition> posMap = {
        {"top", PanelPosition::PANEL_TOP},
        {"bottom", PanelPosition::PANEL_BOTTOM},
        {"left", PanelPosition::PANEL_LEFT},
        {"right", PanelPosition::PANEL_RIGHT}
    };
    QString posString = settingsConf->value("panelPosition", "top").toString();
    
    mCachedPanelPosition = posMap.value(posString, PanelPosition::PANEL_TOP);
    mPanelPositionCacheValid = true;
    return mCachedPanelPosition;
}

void Settings::setPanelPosition(PanelPosition pos) {
    QString posString;
    switch(pos) {
        case PANEL_TOP:
            posString = "top";
            break;
        case PANEL_BOTTOM:
            posString = "bottom";
            break;
        case PANEL_LEFT:
            posString = "left";
            break;
        case PANEL_RIGHT:
            posString = "right";
            break;
    }
    settingsConf->setValue("panelPosition", posString);
    mPanelPositionCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::panelPinned() {
    return settingsConf->value("panelPinned", false).toBool();
}

void Settings::setPanelPinned(bool mode) {
    settingsConf->setValue("panelPinned", mode);
}
//------------------------------------------------------------------------------
/*
 * 0: fit window
 * 1: fit width
 * 2: orginal size
 * 3: fit window (stretch)
 */
ImageFitMode Settings::imageFitMode() {
    if (mImageFitModeCacheValid) {
        return mCachedImageFitMode;
    }
    
    int mode = settingsConf->value("defaultFitMode", 0).toInt();
    if(mode < 0 || mode > 3) {
        qDebug() << "Settings: Invalid fit mode ( " + QString::number(mode) + " ). Resetting to default.";
        mode = 0;
    }
    
    mCachedImageFitMode = static_cast<ImageFitMode>(mode);
    mImageFitModeCacheValid = true;
    return mCachedImageFitMode;
}

void Settings::setImageFitMode(ImageFitMode mode) {
    int modeInt = static_cast<int>(mode);
    if(modeInt < 0 || modeInt > 3) {
        qDebug() << "Settings: Invalid fit mode ( " + QString::number(modeInt) + " ). Resetting to default.";
        modeInt = 0;
    }
    settingsConf->setValue("defaultFitMode", modeInt);
    mImageFitModeCacheValid = false;
}
//------------------------------------------------------------------------------
QRect Settings::windowGeometry() {
    if (mWindowGeometryCacheValid) {
        return mCachedWindowGeometry;
    }
    
    QRect savedRect = stateConf->value("windowGeometry").toRect();
    if(savedRect.size().isEmpty())
        savedRect.setRect(100, 100, 900, 600);
    
    mCachedWindowGeometry = savedRect;
    mWindowGeometryCacheValid = true;
    return savedRect;
}

void Settings::setWindowGeometry(QRect geometry) {
    stateConf->setValue("windowGeometry", geometry);
    mWindowGeometryCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::loopSlideshow() {
    return settingsConf->value("loopSlideshow", false).toBool();
}

void Settings::setLoopSlideshow(bool mode) {
    settingsConf->setValue("loopSlideshow", mode);
}
//------------------------------------------------------------------------------
void Settings::sendChangeNotification() {
    emit settingsChanged();
}
//------------------------------------------------------------------------------
void Settings::readShortcuts(QMap<QString, QString> &shortcuts) {
    if (mShortcutsCacheValid) {
        shortcuts = mCachedShortcuts;
        return;
    }
    
    settingsConf->beginGroup("Controls");
    QStringList in, pair;
    in = settingsConf->value("shortcuts").toStringList();
    for(int i = 0; i < in.count(); i++) {
        pair = in[i].split("=");
        if(pair.size() >= 2 && !pair[0].isEmpty() && !pair[1].isEmpty()) {
            if(pair[1].endsWith("eq"))
                pair[1]=pair[1].chopped(2) + "=";
            shortcuts.insert(pair[1], pair[0]);
        }
    }
    settingsConf->endGroup();
    
    mCachedShortcuts = shortcuts;
    mShortcutsCacheValid = true;
}

void Settings::saveShortcuts(const QMap<QString, QString> &shortcuts) {
    settingsConf->beginGroup("Controls");
    QMapIterator<QString, QString> i(shortcuts);
    QStringList out;
    while(i.hasNext()) {
        i.next();
        if(i.key().endsWith("="))
            out << i.value() + "=" + i.key().chopped(1) + "eq";
        else
            out << i.value() + "=" + i.key();
    }
    settingsConf->setValue("shortcuts", out);
    settingsConf->endGroup();
    mShortcutsCacheValid = false;
}
//------------------------------------------------------------------------------
void Settings::readScripts(QHash<QString, Script> &scripts) {
    if (mScriptsCacheValid) {
        scripts = mCachedScripts;
        return;
    }
    
    scripts.clear();
    settingsConf->beginGroup("Scripts");
    int size = settingsConf->beginReadArray("script");
    for(int i=0; i < size; i++) {
        settingsConf->setArrayIndex(i);
        QString name = settingsConf->value("name").toString();
        QVariant value = settingsConf->value("value");
        Script scr = value.value<Script>();
        scripts.insert(name, scr);
    }
    settingsConf->endArray();
    settingsConf->endGroup();
    
    mCachedScripts = scripts;
    mScriptsCacheValid = true;
}

void Settings::saveScripts(const QHash<QString, Script> &scripts) {
    settingsConf->beginGroup("Scripts");
    settingsConf->beginWriteArray("script");
    QHashIterator<QString, Script> i(scripts);
    int counter = 0;
    while(i.hasNext()) {
        i.next();
        settingsConf->setArrayIndex(counter);
        settingsConf->setValue("name", i.key());
        settingsConf->setValue("value", QVariant::fromValue(i.value()));
        counter++;
    }
    settingsConf->endArray();
    settingsConf->endGroup();
    mScriptsCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::transparencyGrid() {
    return settingsConf->value("drawTransparencyGrid", false).toBool();
}

void Settings::setTransparencyGrid(bool mode) {
    settingsConf->setValue("drawTransparencyGrid", mode);
}
//------------------------------------------------------------------------------
bool Settings::enableSmoothScroll() {
    return settingsConf->value("enableSmoothScroll", true).toBool();
}

void Settings::setEnableSmoothScroll(bool mode) {
    settingsConf->setValue("enableSmoothScroll", mode);
}
//------------------------------------------------------------------------------
QStringList Settings::savedPaths() {
    if (mSavedPathsCacheValid) {
        return mCachedSavedPaths;
    }
    
    mCachedSavedPaths = stateConf->value("savedPaths", QDir::homePath()).toStringList();
    mSavedPathsCacheValid = true;
    return mCachedSavedPaths;
}

void Settings::setSavedPaths(const QStringList &paths) {
    stateConf->setValue("savedPaths", paths);
    mSavedPathsCacheValid = false;
}
//------------------------------------------------------------------------------
QStringList Settings::bookmarks() {
    if (mBookmarksCacheValid) {
        return mCachedBookmarks;
    }
    
    mCachedBookmarks = stateConf->value("bookmarks").toStringList();
    mBookmarksCacheValid = true;
    return mCachedBookmarks;
}

void Settings::setBookmarks(const QStringList &paths) {
    stateConf->setValue("bookmarks", paths);
    mBookmarksCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::placesPanel() {
    return stateConf->value("placesPanel", true).toBool();
}

void Settings::setPlacesPanel(bool mode) {
    stateConf->setValue("placesPanel", mode);
}
//------------------------------------------------------------------------------
bool Settings::placesPanelBookmarksExpanded() {
    return stateConf->value("placesPanelBookmarksExpanded", true).toBool();
}

void Settings::setPlacesPanelBookmarksExpanded(bool mode) {
    stateConf->setValue("placesPanelBookmarksExpanded", mode);
}
//------------------------------------------------------------------------------
bool Settings::placesPanelTreeExpanded() {
    return stateConf->value("placesPanelTreeExpanded", true).toBool();
}

void Settings::setPlacesPanelTreeExpanded(bool mode) {
    stateConf->setValue("placesPanelTreeExpanded", mode);
}
//------------------------------------------------------------------------------
int Settings::placesPanelWidth() {
    return stateConf->value("placesPanelWidth", 260).toInt();
}

void Settings::setPlacesPanelWidth(int width) {
    stateConf->setValue("placesPanelWidth", width);
}
//------------------------------------------------------------------------------
void Settings::setSlideshowInterval(int ms) {
    settingsConf->setValue("slideshowInterval", ms);
}

int Settings::slideshowInterval() {
    int interval = settingsConf->value("slideshowInterval", 3000).toInt();
    if(interval <= 0)
        interval = 3000;
    return interval;
}
//------------------------------------------------------------------------------
bool Settings::smoothUpscaling() {
    return settingsConf->value("smoothUpscaling", true).toBool();
}

void Settings::setSmoothUpscaling(bool mode) {
    settingsConf->setValue("smoothUpscaling", mode);
}
//------------------------------------------------------------------------------
bool Settings::expandImage() {
    return settingsConf->value("expandImage", false).toBool();
}

void Settings::setExpandImage(bool mode) {
    settingsConf->setValue("expandImage", mode);
}
//------------------------------------------------------------------------------
int Settings::expandLimit() {
    return settingsConf->value("expandLimit", 2).toInt();
}

void Settings::setExpandLimit(int value) {
    settingsConf->setValue("expandLimit", value);
}
//------------------------------------------------------------------------------
int Settings::JPEGSaveQuality() {
    if (mJPEGSaveQualityCacheValid) {
        return mCachedJPEGSaveQuality;
    }
    
    int quality = std::clamp(settingsConf->value("JPEGSaveQuality", 95).toInt(), 0, 100);
    mCachedJPEGSaveQuality = quality;
    mJPEGSaveQualityCacheValid = true;
    return quality;
}

void Settings::setJPEGSaveQuality(int value) {
    settingsConf->setValue("JPEGSaveQuality", value);
    mJPEGSaveQualityCacheValid = false;
}
//------------------------------------------------------------------------------
ScalingFilter Settings::scalingFilter() {
    if (mScalingFilterCacheValid) {
        return mCachedScalingFilter;
    }
    
    int defaultFilter = 1;
#ifdef USE_OPENCV
    // default to a nicer QI_FILTER_CV_CUBIC
    defaultFilter = 3;
#endif
    int mode = settingsConf->value("scalingFilter", defaultFilter).toInt();
#ifndef USE_OPENCV
    if(mode > 2)
        mode = 1;
#endif
    if(mode < 0 || mode > 4)
        mode = 1;
    
    mCachedScalingFilter = static_cast<ScalingFilter>(mode);
    mScalingFilterCacheValid = true;
    return mCachedScalingFilter;
}

void Settings::setScalingFilter(ScalingFilter mode) {
    settingsConf->setValue("scalingFilter", mode);
    mScalingFilterCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::smoothAnimatedImages() {
    return settingsConf->value("smoothAnimatedImages", true).toBool();
}

void Settings::setSmoothAnimatedImages(bool mode) {
    settingsConf->setValue("smoothAnimatedImages", mode);
}
//------------------------------------------------------------------------------
bool Settings::infoBarFullscreen() {
    return settingsConf->value("infoBarFullscreen", true).toBool();
}

void Settings::setInfoBarFullscreen(bool mode) {
    settingsConf->setValue("infoBarFullscreen", mode);
}
//------------------------------------------------------------------------------
bool Settings::infoBarWindowed() {
    return settingsConf->value("infoBarWindowed", false).toBool();
}

void Settings::setInfoBarWindowed(bool mode) {
    settingsConf->setValue("infoBarWindowed", mode);
}
//------------------------------------------------------------------------------
bool Settings::windowTitleExtendedInfo() {
    return settingsConf->value("windowTitleExtendedInfo", true).toBool();
}

void Settings::setWindowTitleExtendedInfo(bool mode) {
    settingsConf->setValue("windowTitleExtendedInfo", mode);
}

//------------------------------------------------------------------------------
bool Settings::cursorAutohide() {
    return settingsConf->value("cursorAutohiding", true).toBool();
}

void Settings::setCursorAutohide(bool mode) {
    settingsConf->setValue("cursorAutohiding", mode);
}
//------------------------------------------------------------------------------
bool Settings::firstRun() {
    return settingsConf->value("firstRun", true).toBool();
}

void Settings::setFirstRun(bool mode) {
    settingsConf->setValue("firstRun", mode);
}
//------------------------------------------------------------------------------
bool Settings::showSaveOverlay() {
    return settingsConf->value("showSaveOverlay", true).toBool();
}

void Settings::setShowSaveOverlay(bool mode) {
    settingsConf->setValue("showSaveOverlay", mode);
}
//------------------------------------------------------------------------------
bool Settings::confirmDelete() {
    return settingsConf->value("confirmDelete", true).toBool();
}

void Settings::setConfirmDelete(bool mode) {
    settingsConf->setValue("confirmDelete", mode);
}
//------------------------------------------------------------------------------
bool Settings::confirmTrash() {
    return settingsConf->value("confirmTrash", true).toBool();
}

void Settings::setConfirmTrash(bool mode) {
    settingsConf->setValue("confirmTrash", mode);
}
//------------------------------------------------------------------------------
float Settings::zoomStep() {
    if (mZoomStepCacheValid) {
        return mCachedZoomStep;
    }
    
    bool ok = false;
    float value = settingsConf->value("zoomStep", 0.2f).toFloat(&ok);
    if(!ok)
        value = 0.2f;
    value = qBound(0.01f, value, 0.5f);
    
    mCachedZoomStep = value;
    mZoomStepCacheValid = true;
    return value;
}

void Settings::setZoomStep(float value) {
    value = qBound(0.01f, value, 0.5f);
    settingsConf->setValue("zoomStep", value);
    mZoomStepCacheValid = false;
}
//------------------------------------------------------------------------------
float Settings::mouseScrollingSpeed() {
    if (mMouseScrollingSpeedCacheValid) {
        return mCachedMouseScrollingSpeed;
    }
    
    bool ok = false;
    float value = settingsConf->value("mouseScrollingSpeed", 1.0f).toFloat(&ok);
    if(!ok)
        value = 1.0f;
    value = qBound(0.5f, value, 2.0f);
    
    mCachedMouseScrollingSpeed = value;
    mMouseScrollingSpeedCacheValid = true;
    return value;
}

void Settings::setMouseScrollingSpeed(float value) {
    value = qBound(0.5f, value, 2.0f);
    settingsConf->setValue("mouseScrollingSpeed", value);
    mMouseScrollingSpeedCacheValid = false;
}
//------------------------------------------------------------------------------
void Settings::setZoomIndicatorMode(ZoomIndicatorMode mode) {
    settingsConf->setValue("zoomIndicatorMode", mode);
    mZoomIndicatorModeCacheValid = false;
}

ZoomIndicatorMode Settings::zoomIndicatorMode() {
    if (mZoomIndicatorModeCacheValid) {
        return mCachedZoomIndicatorMode;
    }
    
    int mode = settingsConf->value("zoomIndicatorMode", 0).toInt();
    if(mode < 0 || mode > 2)
        mode = 0;
    
    mCachedZoomIndicatorMode = static_cast<ZoomIndicatorMode>(mode);
    mZoomIndicatorModeCacheValid = true;
    return mCachedZoomIndicatorMode;
}
//------------------------------------------------------------------------------
void Settings::setFocusPointIn1to1Mode(ImageFocusPoint mode) {
    settingsConf->setValue("focusPointIn1to1Mode", mode);
    mFocusPointIn1to1ModeCacheValid = false;
}

ImageFocusPoint Settings::focusPointIn1to1Mode() {
    if (mFocusPointIn1to1ModeCacheValid) {
        return mCachedFocusPointIn1to1Mode;
    }
    
    int mode = settingsConf->value("focusPointIn1to1Mode", 1).toInt();
    if(mode < 0 || mode > 2)
        mode = 1;
    
    mCachedFocusPointIn1to1Mode = static_cast<ImageFocusPoint>(mode);
    mFocusPointIn1to1ModeCacheValid = true;
    return mCachedFocusPointIn1to1Mode;
}

void Settings::setDefaultCropAction(DefaultCropAction mode) {
    settingsConf->setValue("defaultCropAction", mode);
    mDefaultCropActionCacheValid = false;
}

DefaultCropAction Settings::defaultCropAction() {
    if (mDefaultCropActionCacheValid) {
        return mCachedDefaultCropAction;
    }
    
    int mode = settingsConf->value("defaultCropAction", 0).toInt();
    if(mode < 0 || mode > 1)
        mode = 0;
    
    mCachedDefaultCropAction = static_cast<DefaultCropAction>(mode);
    mDefaultCropActionCacheValid = true;
    return mCachedDefaultCropAction;
}

ImageScrolling Settings::imageScrolling() {
    if (mImageScrollingCacheValid) {
        return mCachedImageScrolling;
    }
    
    int mode = settingsConf->value("imageScrolling", 1).toInt();
    if(mode < 0 || mode > 2)
        mode = 0;
    
    mCachedImageScrolling = static_cast<ImageScrolling>(mode);
    mImageScrollingCacheValid = true;
    return mCachedImageScrolling;
}

void Settings::setImageScrolling(ImageScrolling mode) {
    settingsConf->setValue("imageScrolling", mode);
    mImageScrollingCacheValid = false;
}
//------------------------------------------------------------------------------
FolderEndAction Settings::folderEndAction() {
    if (mFolderEndActionCacheValid) {
        return mCachedFolderEndAction;
    }
    
    int mode = settingsConf->value("folderEndAction", 0).toInt();
    if(mode < 0 || mode > 2)
        mode = 0;
    
    mCachedFolderEndAction = static_cast<FolderEndAction>(mode);
    mFolderEndActionCacheValid = true;
    return mCachedFolderEndAction;
}

void Settings::setFolderEndAction(FolderEndAction mode) {
    settingsConf->setValue("folderEndAction", mode);
    mFolderEndActionCacheValid = false;
}
//------------------------------------------------------------------------------
bool Settings::printLandscape() {
    return stateConf->value("printLandscape", false).toBool();
}

void Settings::setPrintLandscape(bool mode) {
    stateConf->setValue("printLandscape", mode);
}
//------------------------------------------------------------------------------
bool Settings::printPdfDefault() {
    return stateConf->value("printPdfDefault", false).toBool();
}

void Settings::setPrintPdfDefault(bool mode) {
    stateConf->setValue("printPdfDefault", mode);
}
//------------------------------------------------------------------------------
bool Settings::printColor() {
    return stateConf->value("printColor", false).toBool();
}

void Settings::setPrintColor(bool mode) {
    stateConf->setValue("printColor", mode);
}
//------------------------------------------------------------------------------
bool Settings::printFitToPage() {
    return stateConf->value("printFitToPage", true).toBool();
}

void Settings::setPrintFitToPage(bool mode) {
    stateConf->setValue("printFitToPage", mode);
}
//------------------------------------------------------------------------------
QString Settings::lastPrinter() {
    return stateConf->value("lastPrinter", "").toString();
}

void Settings::setLastPrinter(const QString &name) {
    stateConf->setValue("lastPrinter", name);
}
//------------------------------------------------------------------------------
bool Settings::jxlAnimation() {
    return settingsConf->value("jxlAnimation", false).toBool();
}

void Settings::setJxlAnimation(bool mode) {
    settingsConf->setValue("jxlAnimation", mode);
}
//------------------------------------------------------------------------------
bool Settings::autoResizeWindow() {
    return settingsConf->value("autoResizeWindow", false).toBool();
}

void Settings::setAutoResizeWindow(bool mode) {
    settingsConf->setValue("autoResizeWindow", mode);
}
//------------------------------------------------------------------------------
int Settings::autoResizeLimit() {
    int limit = settingsConf->value("autoResizeLimit", 90).toInt();
    if(limit < 30 || limit > 100)
        limit = 90;
    return limit;
}

void Settings::setAutoResizeLimit(int percent) {
    settingsConf->setValue("autoResizeLimit", percent);
}
//------------------------------------------------------------------------------
int Settings::memoryAllocationLimit() {
    int limit = settingsConf->value("memoryAllocationLimit", 1024).toInt();
    if(limit < 512)
        limit = 512;
    else if(limit > 8192)
        limit = 8192;
    return limit;
}

void Settings::setMemoryAllocationLimit(int limitMB) {
    settingsConf->setValue("memoryAllocationLimit", limitMB);
}
//------------------------------------------------------------------------------
bool Settings::panelCenterSelection() {
    return settingsConf->value("panelCenterSelection", false).toBool();
}

void Settings::setPanelCenterSelection(bool mode) {
    settingsConf->setValue("panelCenterSelection", mode);
}
//------------------------------------------------------------------------------
QString Settings::language() {
    return settingsConf->value("language", "en_US").toString();
}

void Settings::setLanguage(const QString &lang) {
    settingsConf->setValue("language", lang);
}
//------------------------------------------------------------------------------
bool Settings::useFixedZoomLevels() {
    return settingsConf->value("useFixedZoomLevels", false).toBool();
}

void Settings::setUseFixedZoomLevels(bool mode) {
    settingsConf->setValue("useFixedZoomLevels", mode);
}
//------------------------------------------------------------------------------
QString Settings::defaultZoomLevels() {
    return QString("0.05,0.1,0.125,0.166,0.25,0.333,0.5,0.66,1,1.5,2,3,4,5,6,7,8");
}
QString Settings::zoomLevels() {
    return settingsConf->value("fixedZoomLevels", defaultZoomLevels()).toString();
}

void Settings::setZoomLevels(const QString &levels) {
    settingsConf->setValue("fixedZoomLevels", levels);
}
//------------------------------------------------------------------------------
bool Settings::unlockMinZoom() {
    return settingsConf->value("unlockMinZoom", true).toBool();
}

void Settings::setUnlockMinZoom(bool mode) {
    settingsConf->setValue("unlockMinZoom", mode);
}
//------------------------------------------------------------------------------
bool Settings::sortFolders() {
    return settingsConf->value("sortFolders", true).toBool();
}

void Settings::setSortFolders(bool mode) {
    settingsConf->setValue("sortFolders", mode);
}
//------------------------------------------------------------------------------
bool Settings::trackpadDetection() {
    return settingsConf->value("trackpadDetection", true).toBool();
}

void Settings::setTrackpadDetection(bool mode) {
    settingsConf->setValue("trackpadDetection", mode);
}
//------------------------------------------------------------------------------
bool Settings::clickableEdges() {
    return settingsConf->value("clickableEdges", false).toBool();
}

void Settings::setClickableEdges(bool mode) {
    settingsConf->setValue("clickableEdges", mode);
}
//------------------------------------------------------------------------------
bool Settings::clickableEdgesVisible() {
    return settingsConf->value("clickableEdgesVisible", true).toBool();
}

void Settings::setClickableEdgesVisible(bool mode) {
    settingsConf->setValue("clickableEdgesVisible", mode);
}
//------------------------------------------------------------------------------
