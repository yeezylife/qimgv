#pragma once

#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QtGlobal>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

enum FileOpResult {
    SUCCESS,
    DESTINATION_FILE_EXISTS,
    DESTINATION_DIR_EXISTS,
    SOURCE_NOT_WRITABLE,
    DESTINATION_NOT_WRITABLE,
    SOURCE_DOES_NOT_EXIST,
    DESTINATION_DOES_NOT_EXIST,
    DIRECTORY_NOT_EMPTY,
    NOTHING_TO_DO,
    OTHER_ERROR
};

class FileOperations {
public:
    static void copyFileTo(const QString &srcFilePath, const QString &destDirPath, bool force, FileOpResult &result);
    static void moveFileTo(const QString &srcFilePath, const QString &destDirPath, bool force, FileOpResult &result);
    static void rename(const QString &srcFilePath, const QString &newName, bool force, FileOpResult &result);
    static void removeFile(const QString &filePath, FileOpResult &result);
    static void removeDir(const QString &dirPath, bool recursive, FileOpResult &result);
    // ⭐ 直接调用 QFile::moveToTrash，单行透传不再经私有 Impl（消图谱伪环）
    static void moveToTrash(const QString &filePath, FileOpResult &result);

    static QString decodeResult(const FileOpResult &result);

private:
    static QString generateHash(const QString &str);
    // ⭐ 三处目标存在性检查合并：copy/move/rename 共用，避免 40 行重复
    static bool prepareDest(const QString &destPath, bool force, FileOpResult &result);
};