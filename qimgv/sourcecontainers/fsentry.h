#pragma once
#include <QString>
#include <filesystem>
#include <utility>

// 强类型包装
struct FilePath { QString value; };
struct FileName { QString value; };

class FSEntry {
public:
    FSEntry() noexcept;
    explicit FSEntry(const QString &filePath); // 建议加上 explicit，防止意外的隐式转换

    // 恢复了 const & 修饰符，确保传递大型路径字符串时的性能
    FSEntry(const FilePath &_path, const FileName &_name, std::uintmax_t _size, 
            std::filesystem::file_time_type _modifyTime, bool _isDirectory) noexcept;

    FSEntry(const FilePath &_path, const FileName &_name, std::uintmax_t _size, bool _isDirectory) noexcept;

    FSEntry(const FilePath &_path, const FileName &_name, bool _isDirectory) noexcept;

    bool operator==(const QString &anotherPath) const noexcept;

    QString path, name;
    std::uintmax_t size = 0;
    std::filesystem::file_time_type modifyTime;
    bool isDirectory = false;
};