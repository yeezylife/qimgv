#include "fsentry.h"

FSEntry::FSEntry() noexcept = default;

FSEntry::FSEntry(const QString &filePath) {
    try {
        std::filesystem::directory_entry stdEntry(std::filesystem::path(filePath.toStdWString()));
        const auto &entryPath = stdEntry.path();
        
        this->path = filePath;
        this->name = QString::fromStdWString(entryPath.filename().wstring());
        this->isDirectory = stdEntry.is_directory();
        
        if (!this->isDirectory) {
            this->size = stdEntry.file_size();
            this->modifyTime = stdEntry.last_write_time();
        }
    } catch (const std::filesystem::filesystem_error &) {
        // 静默处理
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

bool FSEntry::operator==(const QString &anotherPath) const noexcept {
    return this->path == anotherPath;
}