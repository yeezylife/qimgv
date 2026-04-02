// wallpapersetter.h
#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include <QString>

class WallpaperSetter
{
public:
    WallpaperSetter();

    // 使用常引用避免不必要的引用计数增减
    static void setWallpaper(const QString &path);
};