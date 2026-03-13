#pragma once

#include <QObject>
#include <QCollator>
#include <QElapsedTimer>
#include <QString>
#include <QSize>
#include <QDebug>
#include <QDateTime>
#include <QRegularExpression>

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>

#include "settings.h"
#include "watchers/directorywatcher.h"
#include "utils/stuff.h"
#include "sourcecontainers/fsentry.h"

#ifdef Q_OS_WIN32
#include "windows.h"
#endif

enum FileListSource { // rename? wip
    SOURCE_DIRECTORY,
    SOURCE_DIRECTORY_RECURSIVE,
    SOURCE_LIST
};

class DirectoryManager;

typedef bool (DirectoryManager::*CompareFunction)(const FSEntry &e1, const FSEntry &e2) const;

//TODO: rename? EntrySomething?

class DirectoryManager : public QObject {
    Q_OBJECT
public:
    DirectoryManager();
    // ignored if the same dir is already opened
    bool setDirectory(const QString &dirPath);
    bool setDirectoryRecursive(const QString &dirPath);
    QString directoryPath() const;
    int indexOfFile(const QString &filePath) const;
    int indexOfDir(const QString &dirPath) const;
    const QString &filePathAt(int index) const;
    unsigned long fileCount() const;
    unsigned long dirCount() const;
    inline bool isSupportedFile(const QString &filePath) const;
    bool isEmpty() const;
    bool containsFile(const QString &filePath) const;
    const QString &fileNameAt(int index) const;
    const QString &prevOfFile(const QString &filePath) const;
    const QString &nextOfFile(const QString &filePath) const;
    const QString &prevOfDir(const QString &dirPath) const;
    const QString &nextOfDir(const QString &dirPath) const;
    void sortEntryLists();
    QDateTime lastModified(const QString &filePath) const;

    const QString &firstFile() const;
    const QString &lastFile() const;
    void setSortingMode(SortingMode mode);
    SortingMode sortingMode() const;
    bool isFile(const QString &path) const;
    bool isDir(const QString &path) const;

    unsigned long totalCount() const;
    bool containsDir(const QString &dirPath) const;
    const FSEntry &fileEntryAt(int index) const;
    const QString &dirPathAt(int index) const;
    const QString &dirNameAt(int index) const;
    bool fileWatcherActive();

    bool insertFileEntry(const QString &filePath);
    bool forceInsertFileEntry(const QString &filePath);
    void removeFileEntry(const QString &filePath);
    void updateFileEntry(const QString &filePath);
    // 定义强类型别名以消除参数混淆警告
    using OldFilePath = const QString&;
    using NewFileName  = const QString&;
    using OldDirPath   = const QString&;
    using NewDirName   = const QString&;

    // 使用结构体包装参数以消除类型混淆警告
    struct FilePath { const QString& value; };
    struct FileName { const QString& value; };
    struct DirPath { const QString& value; };
    struct DirName { const QString& value; };

    void renameFileEntry(FilePath oldFilePath, FileName newFileName);

    bool insertDirEntry(const QString &dirPath);
    //bool forceInsertDirEntry(const QString &dirPath);
    void removeDirEntry(const QString &dirPath);
    //void updateDirEntry(const QString &dirPath);
    void renameDirEntry(DirPath oldDirPath, DirName newDirName);

    FileListSource source() const;

    QStringList fileList() const;

private:
    // 增量排序优化：记录上次排序的比较函数，避免不必要的重新排序
    CompareFunction mLastCompareFunction = nullptr;
    bool mFilesSorted = false;
    bool mDirsSorted = false;
    
    // 增量排序辅助方法
    void sortFileEntryListsIncremental();
    void sortDirEntryListsIncremental();

private:
    QRegularExpression regex;
    QCollator collator;
    std::vector<FSEntry> fileEntryVec, dirEntryVec;
    const FSEntry defaultEntry{};
    QString mDirectoryPath;
    DirectoryWatcher* watcher = nullptr;
    SortingMode mSortingMode = SORT_NAME;
    FileListSource mListSource = SOURCE_DIRECTORY;

    void readSettings();
    void loadEntryList(const QString &directoryPath, bool recursive);

    bool path_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool path_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    bool name_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool name_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    bool date_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool date_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    CompareFunction compareFunction();
    bool size_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool size_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    void startFileWatcher(const QString &directoryPath);
    void stopFileWatcher();

    void addEntriesFromDirectory(std::vector<FSEntry> &entryVec, const QString &directoryPath);
    void addEntriesFromDirectoryRecursive(std::vector<FSEntry> &entryVec, const QString &directoryPath);
    bool checkFileRange(int index) const;
    bool checkDirRange(int index) const;

private slots:
    void onFileAddedExternal(const QString &fileName);
    void onFileRemovedExternal(const QString &fileName);
    void onFileModifiedExternal(const QString &fileName);
    void onFileRenamedExternal(const QString &oldFileName, const QString &newFileName);

signals:
    void loaded(const QString &path);
    void sortingChanged();
    void fileRemoved(QString filePath, int);
    void fileModified(QString filePath);
    void fileAdded(QString filePath);
    void fileRenamed(QString fromPath, int indexFrom, QString toPath, int indexTo);

    void dirRemoved(QString dirPath, int);
    void dirAdded(QString dirPath);
    void dirRenamed(QString fromPath, int indexFrom, QString toPath, int indexTo);
};
