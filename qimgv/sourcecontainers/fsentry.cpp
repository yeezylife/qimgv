#include "fsentry.h"
#include <QFileInfo>
#include <QDateTime>
#include <algorithm>
#include <chrono>

FSEntry::FSEntry() noexcept = default;

QString FSEntry::extractFileName(const QString& path) noexcept {
    qsizetype pos = path.lastIndexOf('/');
#ifdef _WIN32
    pos = std::max(pos, path.lastIndexOf('\\'));
#endif

    if (pos < 0)
        return path;

    return path.mid(pos + 1);
}

// QDateTime(ms) -> file_time_type：仅用于排序与变更比较，毫秒精度足够
static std::filesystem::file_time_type toFileTime(const QDateTime &dt) {
    using namespace std::chrono;
    return file_clock::from_sys(system_clock::time_point(milliseconds(dt.toMSecsSinceEpoch())));
}

std::optional<FSEntry> FSEntry::fromPath(const QString &filePath) {
    // ⭐ QFileInfo 首次访问触发一次 stat 并缓存全部元数据，
    // 取代 directory_entry 构造 + file_size + last_write_time 的 3 次 stat
    QFileInfo fi(filePath);
    if (!fi.exists())
        return std::nullopt;

    FSEntry result;
    result.path = filePath;
    result.name = extractFileName(filePath);
    result.isDirectory = fi.isDir();

    if (!result.isDirectory) {
        result.size = static_cast<std::uintmax_t>(fi.size());
        result.modifyTime = toFileTime(fi.lastModified());
    }

    return result;
}

std::optional<FSEntry> FSEntry::fromPath(const QString &filePath, const QString &name) {
    // 调用方已给出文件名，直接复用，避免 fromPath(filePath) 内又做一次 extractFileName
    QFileInfo fi(filePath);
    if (!fi.exists())
        return std::nullopt;

    FSEntry result;
    result.path = filePath;
    result.name = name;
    result.isDirectory = fi.isDir();

    if (!result.isDirectory) {
        result.size = static_cast<std::uintmax_t>(fi.size());
        result.modifyTime = toFileTime(fi.lastModified());
    }

    return result;
}

bool FSEntry::refresh(const QFileInfo &fi) noexcept {
    // ⭐ 复用外部 QFileInfo，仅比较 modifyTime，变化时更新 size/mtime
    if(fi.isDir() != isDirectory)
        return false;
    if(isDirectory)
        return false;
    const auto newTime = toFileTime(fi.lastModified());
    if(newTime == modifyTime)
        return false;
    modifyTime = newTime;
    size = static_cast<std::uintmax_t>(fi.size());
    return true;
}

FSEntry::FSEntry(const QString &filePath) {
    // 与 fromPath 共用同一套单次 stat 逻辑，避免两处实现漂移
    if (auto entry = fromPath(filePath))
        *this = std::move(*entry);
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
    return path == anotherPath;
}