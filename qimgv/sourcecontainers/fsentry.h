#pragma once

#include <QString>
#include <filesystem>
#include <optional>
#include <utility>

// 强类型包装 - 统一放在此处作为唯一源头
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

    // 性能优化版构造函数：按值传递 + std::move
    FSEntry(FilePath _path, FileName _name, std::uintmax_t _size,
            std::filesystem::file_time_type _modifyTime, bool _isDirectory) noexcept;

    FSEntry(FilePath _path, FileName _name, std::uintmax_t _size, bool _isDirectory) noexcept;

    FSEntry(FilePath _path, FileName _name, bool _isDirectory) noexcept;

    // 静态工厂方法：从路径构造，无异常，返回 optional
    static std::optional<FSEntry> fromPath(const QString &filePath);

    bool operator==(const QString &anotherPath) const noexcept;

    QString path, name;
    std::uintmax_t size = 0;
    std::filesystem::file_time_type modifyTime;
    bool isDirectory = false;
};