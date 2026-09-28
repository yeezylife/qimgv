#include "fileoperations.h"
#include <QFile>
#include <QCryptographicHash>

namespace {

// ✅ 合并 stat，避免重复 QFileInfo 构造
inline bool getFileInfo(const QString& path, QFileInfo& out) noexcept {
    out.setFile(path);
    return out.exists();
}

// ✅ 避免不必要 open（Qt6 已支持直接 setFileTime 静态调用）
inline void restoreFileTimestamps(const QString& path,
                                  const QDateTime& modTime,
                                  const QDateTime& readTime) noexcept
{
    QFile f(path);

    // 🚀 关键：只 open 一次
    if (!f.open(QIODevice::ReadWrite))
        return;

    f.setFileTime(modTime, QFileDevice::FileModificationTime);
    f.setFileTime(readTime, QFileDevice::FileAccessTime);
}

} // namespace

QString FileOperations::generateHash(const QString &str) {
    return QString(QCryptographicHash::hash(str.toUtf8(),
                                            QCryptographicHash::Md5).toHex());
}

bool FileOperations::prepareDest(const QString &destPath, bool force, FileOpResult &result) {
    // ⭐ 返回 true 表示目标可写、调用方可继续；false 表示已设置 result 并应提前返回
    QFileInfo dest(destPath);
    if(!dest.exists())
        return true;
#ifdef Q_OS_WIN32
    if(!dest.isWritable()) {
        result = DESTINATION_NOT_WRITABLE;
        return false;
    }
#endif
    if(dest.isDir()) {
        result = DESTINATION_DIR_EXISTS;
        return false;
    }
    if(!force) {
        result = DESTINATION_FILE_EXISTS;
        return false;
    }
    if(!QFile::remove(destPath)) {
        result = OTHER_ERROR;
        return false;
    }
    return true;
}

void FileOperations::removeFile(const QString &filePath, FileOpResult &result) {
    QFileInfo fi;
    if (!getFileInfo(filePath, fi)) {
        result = SOURCE_DOES_NOT_EXIST;
        return;
    }

#ifdef Q_OS_WIN32
    if (!fi.isWritable()) {
        result = SOURCE_NOT_WRITABLE;
        return;
    }
#endif

    result = QFile::remove(filePath) ? SUCCESS : OTHER_ERROR;
}

void FileOperations::removeDir(const QString &dirPath, bool recursive, FileOpResult &result) {
    QDir dir(dirPath);

    if (!dir.exists()) {
        result = SOURCE_DOES_NOT_EXIST;
        return;
    }

    if (recursive) {
        result = dir.removeRecursively() ? SUCCESS : OTHER_ERROR;
        return;
    }

    if (dir.rmdir(dirPath)) {
        result = SUCCESS;
    } else {
        result = dir.isEmpty() ? OTHER_ERROR : DIRECTORY_NOT_EMPTY;
    }
}

QString FileOperations::decodeResult(const FileOpResult &result) {
    switch (result) {
    case SUCCESS: return QObject::tr("Operation completed succesfully.");
    case DESTINATION_FILE_EXISTS: return QObject::tr("Destination file exists.");
    case DESTINATION_DIR_EXISTS: return QObject::tr("Destination directory exists.");
    case SOURCE_NOT_WRITABLE: return QObject::tr("Source file is not writable.");
    case DESTINATION_NOT_WRITABLE: return QObject::tr("Destination is not writable.");
    case SOURCE_DOES_NOT_EXIST: return QObject::tr("Source file does not exist.");
    case DESTINATION_DOES_NOT_EXIST: return QObject::tr("Destination does not exist.");
    case DIRECTORY_NOT_EMPTY: return QObject::tr("Directory is not empty.");
    case NOTHING_TO_DO: return QObject::tr("Nothing to do.");
    default: return QObject::tr("Other error.");
    }
}

void FileOperations::copyFileTo(const QString &srcPath,
                               const QString &destDirPath,
                               bool force,
                               FileOpResult &result)
{
    QFileInfo src;
    if (!getFileInfo(srcPath, src)) {
        result = SOURCE_DOES_NOT_EXIST;
        return;
    }

    const QString srcDir = src.absolutePath();
    if (destDirPath == srcDir) {
        result = NOTHING_TO_DO;
        return;
    }

    QFileInfo dirInfo(destDirPath);
    if (!dirInfo.exists() || !dirInfo.isDir() || !dirInfo.isWritable()) {
        result = DESTINATION_NOT_WRITABLE;
        return;
    }

    const QString destPath = QDir(destDirPath).filePath(src.fileName());
    if(!prepareDest(destPath, force, result))
        return;

    const auto modTime = src.lastModified();
    const auto readTime = src.lastRead();

    if (QFile::copy(srcPath, destPath)) {
        restoreFileTimestamps(destPath, modTime, readTime);
        result = SUCCESS;
    } else {
        result = OTHER_ERROR;
    }
}

void FileOperations::moveFileTo(const QString &srcPath,
                               const QString &destDirPath,
                               bool force,
                               FileOpResult &result)
{
    QFileInfo src;
    if (!getFileInfo(srcPath, src) || !src.isWritable()) {
        result = SOURCE_NOT_WRITABLE;
        return;
    }

    const QString srcDir = src.absolutePath();
    if (destDirPath == srcDir) {
        result = NOTHING_TO_DO;
        return;
    }

    QFileInfo dirInfo(destDirPath);
    if (!dirInfo.exists() || !dirInfo.isDir() || !dirInfo.isWritable()) {
        result = DESTINATION_NOT_WRITABLE;
        return;
    }

    const QString destPath = QDir(destDirPath).filePath(src.fileName());

    // 🚀 1️⃣ rename 快路径（避免 copy）
    if (QFile::rename(srcPath, destPath)) {
        result = SUCCESS;
        return;
    }

    if(!prepareDest(destPath, force, result))
        return;

    const auto modTime = src.lastModified();
    const auto readTime = src.lastRead();

    if (QFile::copy(srcPath, destPath)) {
        if (QFile::remove(srcPath)) {
            restoreFileTimestamps(destPath, modTime, readTime);
            result = SUCCESS;
            return;
        }
        QFile::remove(destPath);
    }

    result = OTHER_ERROR;
}

void FileOperations::rename(const QString &srcPath,
                            const QString &newName,
                            bool force,
                            FileOpResult &result)
{
    QFileInfo src;
    if (!getFileInfo(srcPath, src) || !src.isWritable()) {
        result = SOURCE_NOT_WRITABLE;
        return;
    }

    if (newName.isEmpty() || newName == src.fileName()) {
        result = NOTHING_TO_DO;
        return;
    }

    const QString destPath = QDir(src.absolutePath()).filePath(newName);
    if(!prepareDest(destPath, force, result))
        return;

    result = QFile::rename(srcPath, destPath) ? SUCCESS : OTHER_ERROR;
}

void FileOperations::moveToTrash(const QString &filePath, FileOpResult &result) {
    QFileInfo fi;
    if (!getFileInfo(filePath, fi) || !fi.isWritable()) {
        result = SOURCE_NOT_WRITABLE;
        return;
    }

    result = QFile::moveToTrash(filePath) ? SUCCESS : OTHER_ERROR;
}