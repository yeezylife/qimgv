#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include <QString>
#include <QProcess>
#include <QDebug>
#include "utils/stuff.h"

class WallpaperSetter
{
public:
    WallpaperSetter();

    // 修改为常引用，避免在静态方法调用时产生不必要的引用计数增减
    static void setWallpaper(const QString &path);
};