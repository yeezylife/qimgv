// wallpapersetter.cpp
#include "wallpapersetter.h"
#include <QProcess>
#include <QDebug>

WallpaperSetter::WallpaperSetter() = default;

void WallpaperSetter::setWallpaper(const QString &path)
{
#ifdef _WIN32
    bool ok = SystemParametersInfo(SPI_SETDESKWALLPAPER,
                                   0,
                                   (PVOID)path.utf16(),
                                   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
    qDebug() << "wallpaper changed:" << (ok ? "true" : "false");
#elif defined(__linux__) || defined(__FreeBSD__)
    // 直接调用 qdbus，无需额外的 shell 进程
    QString script = QStringLiteral(
        "var allDesktops = desktops();"
        "for (i = 0; i < allDesktops.length; i++) {"
        "    d = allDesktops[i];"
        "    d.wallpaperPlugin = \"org.kde.image\";"
        "    d.currentConfigGroup = Array(\"Wallpaper\", \"org.kde.image\", \"General\");"
        "    d.writeConfig(\"Image\", \"%1\");"
        "}"
    ).arg(path);

    QProcess process;
    process.start(QStringLiteral("qdbus"),
                  QStringList() << QStringLiteral("org.kde.plasmashell")
                                << QStringLiteral("/PlasmaShell")
                                << QStringLiteral("org.kde.PlasmaShell.evaluateScript")
                                << script);
    process.waitForFinished();
    qDebug() << "In case that didn't work, your cropped wallpaper is saved at:" << path;
#endif
}