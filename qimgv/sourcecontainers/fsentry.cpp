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
        // 处理异常：保持对象为默认状态，避免空 catch 触发 clang-tidy 警告
        // qimgv 的语义是“静默失败”，因此不记录日志、不抛出异常
        this->path = filePath;
        this->name = QString();
        this->isDirectory = false;
        this->size = 0;
        // modifyTime 保持默认值
        (void)err; // 明确使用 err，避免 unused-variable 警告
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
