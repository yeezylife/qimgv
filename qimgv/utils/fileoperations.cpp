#include "fileoperations.h"
#include <QDir>
#include <QFile>
#include <QCryptographicHash>

namespace {

inline bool isFileValid(const QString& path, bool checkWritable = false) noexcept {
    QFileInfo file(path);
    if (!file.exists())
        return false;
    if (checkWritable && !file.isWritable())
        return false;
    return true;
}

inline void restoreFileTimestamps(const QString& path,
                                  const QDateTime& modTime,
                                  const QDateTime& readTime) noexcept {
    QFile f(path);
    if (f.open(QIODevice::ReadWrite)) {
        f.setFileTime(modTime, QFileDevice::FileModificationTime);
        f.setFileTime(readTime, QFileDevice::FileAccessTime);
    }
}

} // namespace

QString FileOperations::generateHash(const QString &str) {
    return QString(QCryptographicHash::hash(str.toUtf8(),
                                            QCryptographicHash::Md5).toHex());
}

void FileOperations::removeFile(const QString &filePath, FileOpResult &result) {
    QFileInfo file(filePath);

    if (!file.exists()) {
        result = SOURCE_DOES_NOT_EXIST;
        return;
    }

#ifdef Q_OS_WIN32
    if (!file.isWritable()) {
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
    QFileInfo src(srcPath);
    const QString srcDir = src.absolutePath();

    if (destDirPath == srcDir) {
        result = NOTHING_TO_DO;
        return;
    }

    if (!isFileValid(srcPath)) {
        result = SOURCE_DOES_NOT_EXIST;
        return;
    }

    // 🔧 修复开始
    QFileInfo dirInfo(destDirPath);
    if (!dirInfo.exists() || !dirInfo.isDir() || !dirInfo.isWritable()) {
        result = DESTINATION_NOT_WRITABLE;
        return;
    }

    QDir targetDir(destDirPath);
    // 🔧 修复结束

    const QString destPath = targetDir.filePath(src.fileName());
    QFileInfo dest(destPath);

    if (dest.exists()) {
#ifdef Q_OS_WIN32
        if (!dest.isWritable()) {
            result = DESTINATION_NOT_WRITABLE;
            return;
        }
#endif
        if (dest.isDir()) {
            result = DESTINATION_DIR_EXISTS;
            return;
        }
        if (!force) {
            result = DESTINATION_FILE_EXISTS;
            return;
        }

        if (!QFile::remove(destPath)) {
            result = OTHER_ERROR;
            return;
        }
    }

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
    QFileInfo src(srcPath);
    const QString srcDir = src.absolutePath();

    if (destDirPath == srcDir) {
        result = NOTHING_TO_DO;
        return;
    }

    if (!isFileValid(srcPath, true)) {
        result = SOURCE_NOT_WRITABLE;
        return;
    }

    // 🔧 修复开始
    QFileInfo dirInfo(destDirPath);
    if (!dirInfo.exists() || !dirInfo.isDir() || !dirInfo.isWritable()) {
        result = DESTINATION_NOT_WRITABLE;
        return;
    }

    QDir targetDir(destDirPath);
    // 🔧 修复结束

    const QString destPath = targetDir.filePath(src.fileName());
    QFileInfo dest(destPath);

    // ========= 1️⃣ rename 快路径（最优） =========
    if (QFile::rename(srcPath, destPath)) {
        result = SUCCESS;
        return;
    }

    // ========= 2️⃣ 处理目标冲突 =========
    if (dest.exists()) {
#ifdef Q_OS_WIN32
        if (!dest.isWritable()) {
            result = DESTINATION_NOT_WRITABLE;
            return;
        }
#endif
        if (dest.isDir()) {
            result = DESTINATION_DIR_EXISTS;
            return;
        }
        if (!force) {
            result = DESTINATION_FILE_EXISTS;
            return;
        }

        if (!QFile::remove(destPath)) {
            result = OTHER_ERROR;
            return;
        }
    }

    // ========= 3️⃣ fallback: copy + remove =========
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
    QFileInfo src(srcPath);

    if (!isFileValid(srcPath, true)) {
        result = SOURCE_NOT_WRITABLE;
        return;
    }

    if (newName.isEmpty() || newName == src.fileName()) {
        result = NOTHING_TO_DO;
        return;
    }

    const QString destPath = QDir(src.absolutePath()).filePath(newName);
    QFileInfo dest(destPath);

    if (dest.exists()) {
#ifdef Q_OS_WIN32
        if (!dest.isWritable()) {
            result = DESTINATION_NOT_WRITABLE;
            return;
        }
#endif
        if (dest.isDir()) {
            result = DESTINATION_DIR_EXISTS;
            return;
        }
        if (!force) {
            result = DESTINATION_FILE_EXISTS;
            return;
        }

        if (!QFile::remove(destPath)) {
            result = OTHER_ERROR;
            return;
        }
    }

    result = QFile::rename(srcPath, destPath) ? SUCCESS : OTHER_ERROR;
}

void FileOperations::moveToTrash(const QString &filePath, FileOpResult &result) {
    if (!isFileValid(filePath, true)) {
        result = SOURCE_NOT_WRITABLE;
        return;
    }

    result = moveToTrashImpl(filePath) ? SUCCESS : OTHER_ERROR;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)

bool FileOperations::moveToTrashImpl(const QString &filePath) {
    return QFile::moveToTrash(filePath);
}

#else

#ifdef Q_OS_WIN32

bool FileOperations::moveToTrashImpl(const QString &file) {
    QFileInfo fi(file);
    if (!fi.exists())
        return false;

    std::wstring w = fi.absoluteFilePath().toStdWString();
    w.push_back(L'\0');

    SHFILEOPSTRUCTW op{};
    op.wFunc = FO_DELETE;
    op.pFrom = w.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    return SHFileOperationW(&op) == 0;
}

#endif
#endif