#include "fsentry.h"

FSEntry::FSEntry() noexcept = default;

FSEntry::FSEntry(const QString &path) {
    try {
        std::filesystem::directory_entry stdEntry(std::filesystem::path(path.toStdWString()));
        const auto &entryPath = stdEntry.path();
        
        this->path = path;
        this->name = QString::fromStdWString(entryPath.filename().wstring());
        this->isDirectory = stdEntry.is_directory();
        
        if (!this->isDirectory) {
            this->size = stdEntry.file_size();
            this->modifyTime = stdEntry.last_write_time();
        }
    } catch (const std::filesystem::filesystem_error &) {
        // 保持原有的静默错误处理行为
    }
}

FSEntry::FSEntry(const QString &_path, const QString &_name, std::uintmax_t _size, std::filesystem::file_time_type _modifyTime, bool _isDirectory) noexcept
    : path(_path),
      name(_name),
      size(_size),
      modifyTime(_modifyTime),
      isDirectory(_isDirectory)
{
}

FSEntry::FSEntry(const QString &_path, const QString &_name, std::uintmax_t _size, bool _isDirectory) noexcept
    : path(_path),
      name(_name),
      size(_size),
      isDirectory(_isDirectory)
{
}

FSEntry::FSEntry(const QString &_path, const QString &_name, bool _isDirectory) noexcept
    : path(_path),
      name(_name),
      isDirectory(_isDirectory)
{
}

bool FSEntry::operator==(const QString &anotherPath) const noexcept {
    return this->path == anotherPath;
}
