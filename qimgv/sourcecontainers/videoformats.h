#pragma once

#include <QMultiMap>
#include <QSet>
#include <QByteArray>

// 视频格式映射表：硬编码常量，构造后永不修改，全树唯一来源
// 任意线程并发只读安全，worker 线程可直接使用，无需经 Settings 中转
// 注意：video/mp4 对应双后缀（mp4/m4v），constFind/value() 取后插入者 m4v 为 canonical，与旧表顺序一致
inline const QMultiMap<QByteArray, QByteArray> &videoFormatTable() {
    static const QMultiMap<QByteArray, QByteArray> table{
        {"video/webm", "webm"},
        {"video/mp4", "mp4"},
        {"video/mp4", "m4v"},
        {"video/mpeg", "mpg"},
        {"video/mpeg", "mpeg"},
        {"video/x-matroska", "mkv"},
        {"video/x-ms-wmv", "wmv"},
        {"video/x-msvideo", "avi"},
        {"video/quicktime", "mov"},
        {"video/x-flv", "flv"},
    };
    return table;
}

// 后缀集合：由上表派生，同样不可变、并发只读安全
inline const QSet<QByteArray> &videoFormatSuffixes() {
    static const QSet<QByteArray> suffixes = []() {
        QSet<QByteArray> set;
        const auto formats = videoFormatTable().values();
        set.reserve(formats.size());
        for (const auto &fmt : formats)
            set.insert(fmt);
        return set;
    }();
    return suffixes;
}
