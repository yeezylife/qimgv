#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <QTimer>
#include <QDir>
#include <QEvent>
#include <memory>
#include <cstdlib>

#ifdef __GLIBC__
#include <malloc.h>
#endif

#include "appversion.h"
#include "settings.h"
#include "components/actionmanager/actionmanager.h"
#include "utils/inputmap.h"
#include "utils/actions.h"
#include "utils/cmdoptionsrunner.h"
#include "sharedresources.h"
#include "proxystyle.h"
#include "core.h"

#ifdef __APPLE__
#include "macosapplication.h"
#endif

//------------------------------------------------------------------------------
void saveSettings() {
    if (settings) {
        delete settings;
        settings = nullptr;
    }
}
//------------------------------------------------------------------------------
QDataStream& operator<<(QDataStream& out, const Script& v) {
    out << v.command << v.blocking;
    return out;
}
//------------------------------------------------------------------------------
QDataStream& operator>>(QDataStream& in, Script& v) {
    in >> v.command;
    in >> v.blocking;
    return in;
}
//------------------------------------------------------------------------------
int main(int argc, char *argv[]) {

    // force some env variables

#ifdef _WIN32
    // if this is set by other app, platform plugin may fail to load
    // https://github.com/easymodo/qimgv/issues/410  
    qputenv("QT_PLUGIN_PATH","");
#endif

    // for hidpi testing
    //qputenv("QT_SCALE_FACTOR","1.5");
    //qputenv("QT_SCREEN_SCALE_FACTORS", "1;1.7");

    // do we still need this?
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR","0");

#if (QT_VERSION_MAJOR == 5)
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // Qt6 hidpi rendering on windows still has artifacts
    // This disables it for scale factors < 1.75
    // In this case only fonts are scaled
#ifdef _WIN32
#if (QT_VERSION_MAJOR == 6)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#endif
#endif

    //qDebug() << qgetenv("QT_SCALE_FACTOR");
    //qDebug() << qgetenv("QT_SCREEN_SCALE_FACTORS");
    //qDebug() << qgetenv("QT_ENABLE_HIGHDPI_SCALING");

#ifdef __APPLE__
    MacOSApplication a(argc, argv);
    // default to "fusion" if available ("macos" has layout bugs, weird comboboxes etc)
    if(QStyleFactory::keys().contains("Fusion"))
        a.setStyle(QStyleFactory::create("Fusion"));
#else
    QApplication a(argc, argv);
    // Qt 接管 ProxyStyle 所有权，程序退出时自动删除
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    a.setStyle(new ProxyStyle());
#endif

    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
    QGuiApplication::setDesktopFileName(QCoreApplication::applicationName() + ".desktop");

    // needed for mpv
    // 设置线程安全的数值格式，确保浮点数解析一致性
    QLocale::setDefault(QLocale::c());

#ifdef __GLIBC__
    // default value of 128k causes memory fragmentation issues
    mallopt(M_MMAP_THRESHOLD, 64000);
#endif

    // use custom types in signals
    qRegisterMetaType<ScalerRequest>("ScalerRequest");
    qRegisterMetaType<Script>("Script");
    qRegisterMetaType<std::shared_ptr<Image>>("std::shared_ptr<Image>");
    qRegisterMetaType<std::shared_ptr<Thumbnail>>("std::shared_ptr<Thumbnail>");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<Script>("Script");
#endif

    // globals
    inputMap = InputMap::getInstance();
    appActions = Actions::getInstance();
    settings = Settings::getInstance();
    scriptManager = ScriptManager::getInstance();
    actionManager = ActionManager::getInstance();

    // 设置 Qt 应用程序信息（移到 Settings 初始化之后）
    QCoreApplication::setOrganizationName("qimgv");
    QCoreApplication::setOrganizationDomain("github.com/easymodo/qimgv");
    QCoreApplication::setApplicationName("qimgv");
    QCoreApplication::setApplicationVersion(appVersion.toString());

    atexit(saveSettings);

// parse args ------------------------------------------------------------------
    QCommandLineParser parser;
    QString appDescription = qApp->applicationName() + " - Fast and configurable image viewer.";
    appDescription.append("\nVersion: " + qApp->applicationVersion());
    appDescription.append("\nLicense: GNU GPLv3");
    parser.setApplicationDescription(appDescription);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("path", QCoreApplication::translate("main", "File or directory path."));
    parser.addOptions({
        {"gen-thumbs",
            QCoreApplication::translate("main", "Generate all thumbnails for directory."),
            QCoreApplication::translate("main", "directory-path")},
        {"gen-thumbs-size",
            QCoreApplication::translate("main", "Thumbnail size. Current size is used if not specified."),
            QCoreApplication::translate("main", "thumbnail-size")},
        {"build-options",
            QCoreApplication::translate("main", "Show build options.")},
    });
    parser.process(a);

    if(parser.isSet("build-options")) {
        auto r = std::make_unique<CmdOptionsRunner>();
        // 确保 runner 在事件循环期间存活
        QTimer::singleShot(0, [r = std::move(r)]() mutable {
            r->showBuildOptions();
        });
        return a.exec();
    }
    if(parser.isSet("gen-thumbs")) {
        int size = settings->folderViewIconSize();
        if(parser.isSet("gen-thumbs-size")) {
            bool ok;
            int parsedSize = parser.value("gen-thumbs-size").toInt(&ok);
            if (ok && parsedSize > 0)
                size = parsedSize;
        }

        auto r = std::make_unique<CmdOptionsRunner>();
        QString dirPath = parser.value("gen-thumbs");
        QTimer::singleShot(0, [r = std::move(r), dirPath, size]() mutable {
            r->generateThumbs(dirPath, size);
        });
        return a.exec();
    }

// -----------------------------------------------------------------------------

    Core core;

#ifdef __APPLE__
    QObject::connect(&a, &MacOSApplication::fileOpened, &core, &Core::loadPath);
#endif

    if(parser.positionalArguments().count())
        core.loadPath(parser.positionalArguments().at(0));
    else if(settings->defaultViewMode() == MODE_FOLDERVIEW)
        core.loadPath(QDir::homePath());

    // defer GUI show until the event loop is running, avoiding manual processEvents call
    QTimer::singleShot(0, &core, &Core::showGui);

    return a.exec();
}