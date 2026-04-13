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

    static std::optional<FSEntry> fromPath(const QString &filePath);

    bool operator==(const QString &anotherPath) const noexcept;

    QString path;
    QString name;

    std::uintmax_t size = 0;
    std::filesystem::file_time_type modifyTime;
    bool isDirectory = false;

private:
    static QString extractFileName(const QString& path) noexcept;
};