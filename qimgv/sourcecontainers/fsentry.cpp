#include "fsentry.h"

FSEntry::FSEntry() noexcept = default;

FSEntry::FSEntry(const QString &filePath) {
    try {
        std::filesystem::directory_entry stdEntry(
            std::filesystem::path(filePath.toStdWString())
        );

        const auto &entryPath = stdEntry.path();

        this->path = filePath;
        this->name = QString::fromStdWString(entryPath.filename().wstring());
        this->isDirectory = stdEntry.is_directory();

        if (!this->isDirectory) {
            this->size = stdEntry.file_size();
            this->modifyTime = stdEntry.last_write_time();
        }
    } catch (const std::filesystem::filesystem_error &err) {
        // 静默失败，保持默认状态
        this->path = filePath;
        this->name = QString();
        this->isDirectory = false;
        this->size = 0;
        (void)err; // 避免未使用参数警告
    }
}

FSEntry::FSEntry(FilePath _path, FileName _name, std::uintmax_t _size,
                 std::filesystem::file_time_type _modifyTime, bool _isDirectory) noexcept
    : path(std::move(_path.value)),
      name(std::move(_name.value)),
      size(_size),
      modifyTime(_modifyTime),
      isDirectory(_isDirectory)
{}

FSEntry::FSEntry(FilePath _path, FileName _name, std::uintmax_t _size, bool _isDirectory) noexcept
    : path(std::move(_path.value)),
      name(std::move(_name.value)),
      size(_size),
      isDirectory(_isDirectory)
{}

FSEntry::FSEntry(FilePath _path, FileName _name, bool _isDirectory) noexcept
    : path(std::move(_path.value)),
      name(std::move(_name.value)),
      isDirectory(_isDirectory)
{}

std::optional<FSEntry> FSEntry::fromPath(const QString &filePath) {
    std::error_code ec;
    std::filesystem::path p(filePath.toStdWString());

    // 使用 directory_entry 一次性获取状态（避免多次系统调用）
    std::filesystem::directory_entry entry(p, ec);
    if (ec) {
        return std::nullopt;
    }

    // 检查是否存在（directory_entry 构造失败已设置 ec，此处可省略，但为安全保留）
    if (!entry.exists(ec) || ec) {
        return std::nullopt;
    }

    FSEntry result;
    result.path = filePath;
    result.name = QString::fromStdWString(p.filename().wstring());
    result.isDirectory = entry.is_directory(ec);
    if (ec) {
        return std::nullopt;
    }

    if (!result.isDirectory) {
        result.size = entry.file_size(ec);
        if (ec) return std::nullopt;
        result.modifyTime = entry.last_write_time(ec);
        if (ec) return std::nullopt;
    }

    return result;
}

bool FSEntry::operator==(const QString &anotherPath) const noexcept {
    return this->path == anotherPath;
}