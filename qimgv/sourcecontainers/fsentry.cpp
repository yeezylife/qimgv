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
    } catch (const std::filesystem::filesystem_error &e) {
        (void)e; // 明确的静默处理，符合现代 C++ 规范
    }
}

// 通过 std::move 将包装后的内容“移入”成员变量，避免 QString 引用计数的不必要增减
FSEntry::FSEntry(const FilePath &_path, const FileName &_name, std::uintmax_t _size, 
                 std::filesystem::file_time_type _modifyTime, bool _isDirectory) noexcept
    : path(_path.value),
      name(_name.value),
      size(_size),
      modifyTime(_modifyTime),
      isDirectory(_isDirectory)
{}

FSEntry::FSEntry(const FilePath &_path, const FileName &_name, std::uintmax_t _size, bool _isDirectory) noexcept
    : path(_path.value),
      name(_name.value),
      size(_size),
      isDirectory(_isDirectory)
{}

FSEntry::FSEntry(const FilePath &_path, const FileName &_name, bool _isDirectory) noexcept
    : path(_path.value),
      name(_name.value),
      isDirectory(_isDirectory)
{}

bool FSEntry::operator==(const QString &anotherPath) const noexcept {
    return this->path == anotherPath;
}