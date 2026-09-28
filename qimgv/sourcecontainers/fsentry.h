#pragma once

#include <QString>
#include <filesystem>
#include <optional>
#include <utility>

struct FilePath {
    QString value;
    explicit FilePath(QString v) : value(std::move(v)) {}
    operator const QString&() const { return value; }
};

struct FileName {
    QString value;
    explicit FileName(QString v) : value(std::move(v)) {}
    operator const QString&() const { return value; }
};

struct DirPath {
    QString value;
    explicit DirPath(QString v) : value(std::move(v)) {}
    operator const QString&() const { return value; }
};

struct DirName {
    QString value;
    explicit DirName(QString v) : value(std::move(v)) {}
    operator const QString&() const { return value; }
};

class FSEntry {
public:
    FSEntry() noexcept;
    explicit FSEntry(const QString &filePath);

    FSEntry(FilePath _path, FileName _name, std::uintmax_t _size,
            std::filesystem::file_time_type _modifyTime, bool _isDirectory) noexcept;

    FSEntry(FilePath _path, FileName _name, std::uintmax_t _size, bool _isDirectory) noexcept;

    FSEntry(FilePath _path, FileName _name, bool _isDirectory) noexcept;

    // 单次 stat 取齐类型/大小/修改时间（QFileInfo 内部缓存），供各调用方复用
    static std::optional<FSEntry> fromPath(const QString &filePath);
    // 复用目录枚举已得到的文件名，避免二次解析路径
    static std::optional<FSEntry> fromPath(const QString &filePath, const QString &name);
    // 用已 stat 的 QFileInfo 原地刷新元数据，返回 modifyTime 是否变化
    bool refresh(const QFileInfo &fi) noexcept;

    static QString extractFileName(const QString& path) noexcept;

    bool operator==(const QString &anotherPath) const noexcept;

    QString path;
    QString name;

    std::uintmax_t size = 0;
    std::filesystem::file_time_type modifyTime;
    bool isDirectory = false;
};