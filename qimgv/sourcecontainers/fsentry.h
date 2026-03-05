#pragma once
#include <QString>
#include <filesystem>
#include "utils/stuff.h"

class FSEntry {
public:
    FSEntry() noexcept;
    FSEntry(const QString &filePath);
    FSEntry(const QString &_path, const QString &_name, std::uintmax_t _size, std::filesystem::file_time_type _modifyTime, bool _isDirectory) noexcept;
    FSEntry(const QString &_path, const QString &_name, std::uintmax_t _size, bool _isDirectory) noexcept;
    FSEntry(const QString &_path, const QString &_name, bool _isDirectory) noexcept;
    bool operator==(const QString &anotherPath) const noexcept;

    QString path, name;
    std::uintmax_t size = 0;
    std::filesystem::file_time_type modifyTime;
    bool isDirectory = false;
};
