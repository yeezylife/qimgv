#include "fsentry.h"

FSEntry::FSEntry() noexcept = default;

static std::filesystem::path toFsPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(
        reinterpret_cast<const wchar_t*>(path.utf16())
    );
#else
    return std::filesystem::path(path.toStdString());
#endif
}

QString FSEntry::extractFileName(const QString& path) noexcept {
    qsizetype pos = path.lastIndexOf('/');
#ifdef _WIN32
    pos = std::max(pos, path.lastIndexOf('\\'));
#endif

    if (pos < 0)
        return path;

    return path.mid(pos + 1);
}

FSEntry::FSEntry(const QString &filePath)
{
    std::error_code ec;

    auto p = toFsPath(filePath);
    std::filesystem::directory_entry entry(p, ec);

    if (ec)
        return;

    path = filePath;
    name = extractFileName(filePath);

    isDirectory = entry.is_directory(ec);
    if (ec)
        return;

    if (!isDirectory) {
        size = entry.file_size(ec);
        if (ec) return;

        modifyTime = entry.last_write_time(ec);
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

std::optional<FSEntry> FSEntry::fromPath(const QString &filePath)
{
    std::error_code ec;

    auto p = toFsPath(filePath);

    std::filesystem::directory_entry entry(p, ec);
    if (ec)
        return std::nullopt;

    FSEntry result;

    result.path = filePath;
    result.name = extractFileName(filePath);

    result.isDirectory = entry.is_directory(ec);
    if (ec)
        return std::nullopt;

    if (!result.isDirectory) {
        result.size = entry.file_size(ec);
        if (ec) return std::nullopt;

        result.modifyTime = entry.last_write_time(ec);
        if (ec) return std::nullopt;
    }

    return result;
}

bool FSEntry::operator==(const QString &anotherPath) const noexcept {
    return path == anotherPath;
}