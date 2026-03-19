#pragma once

#include <QObject>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QRegularExpression>
#include <QCollator>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <ranges>
#include <filesystem>
#include "settings.h"
#include "watchers/directorywatcher.h"
#include "utils/stuff.h"
#include "sourcecontainers/fsentry.h"

#ifdef Q_OS_WIN32
#include "windows.h"
#endif

enum FileListSource {
    // rename? wip
    SOURCE_DIRECTORY,
    SOURCE_DIRECTORY_RECURSIVE,
    SOURCE_LIST
};

class DirectoryManager;

// 类内定义类型别名，修复编译错误
class DirectoryManager : public QObject {
    Q_OBJECT
public:
    // 类型别名 - 类内定义（修复编译错误关键）
    using CompareFunction = bool (DirectoryManager::*)(const FSEntry &e1, const FSEntry &e2) const;

    DirectoryManager();

    // ignored if the same dir is already opened
    bool setDirectory(const QString &dirPath);
    bool setDirectoryRecursive(const QString &dirPath);
    
    // 内联优化：移至头文件
    inline QString directoryPath() const {
        if(mListSource == SOURCE_DIRECTORY || mListSource == SOURCE_DIRECTORY_RECURSIVE)
            return mDirectoryPath;
        return "";
    }

    int indexOfFile(const QString &filePath) const;
    int indexOfDir(const QString &dirPath) const;
    const QString &filePathAt(int index) const;
    
    // 内联优化：移至头文件
    inline unsigned long fileCount() const { return fileEntryVec.size(); }
    inline unsigned long dirCount() const { return dirEntryVec.size(); }
    inline unsigned long totalCount() const { return fileCount() + dirCount(); }

    inline bool isSupportedFile(const QString &filePath) const;
    
    // 内联优化：移至头文件
    inline bool isEmpty() const { return fileEntryVec.empty(); }

    // 内联优化：contains 方法使用哈希表查找 O(1)
    inline bool containsFile(const QString &filePath) const {
        return mFileIndexMap.contains(filePath);
    }

    inline bool containsDir(const QString &dirPath) const {
        return mDirIndexMap.contains(dirPath);
    }

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
    
    // 内联优化：移至头文件
    inline SortingMode sortingMode() const { return mSortingMode; }

    bool isFile(const QString &path) const;
    bool isDir(const QString &path) const;
    const FSEntry &fileEntryAt(int index) const;
    const QString &dirPathAt(int index) const;
    const QString &dirNameAt(int index) const;
    
    // 内联优化：移至头文件
    inline bool fileWatcherActive() {
        if(!watcher) return false;
        return watcher->isObserving();
    }

    bool insertFileEntry(const QString &filePath);
    bool forceInsertFileEntry(const QString &filePath);
    void removeFileEntry(const QString &filePath);
    void updateFileEntry(const QString &filePath);
    void renameFileEntry(const FilePath &oldFilePath, const FileName &newFileName);
    bool insertDirEntry(const QString &dirPath);
    void removeDirEntry(const QString &dirPath);
    void renameDirEntry(const DirPath &oldDirPath, const DirName &newDirName);

    // 内联优化：移至头文件
    inline FileListSource source() const { return mListSource; }

    QStringList fileList() const;

private:
    // 增量排序优化：记录上次排序的比较函数，避免不必要的重新排序
    CompareFunction mLastCompareFunction = nullptr;
    bool mFilesSorted = false;
    bool mDirsSorted = false;

    // 增量排序辅助方法
    void sortFileEntryListsIncremental();
    void sortDirEntryListsIncremental();

    // 哈希索引映射 - 性能优化核心 (O(n) → O(1))
    std::unordered_map<QString, int> mFileIndexMap;
    std::unordered_map<QString, int> mDirIndexMap;

    // 索引映射维护方法
    void rebuildFileIndexMap();
    void rebuildDirIndexMap();
    void updateFileIndexAfterInsert(const QString &path, int index);
    void updateFileIndexAfterRemove(const QString &path);
    void updateDirIndexAfterInsert(const QString &path, int index);
    void updateDirIndexAfterRemove(const QString &path);

private:
    QRegularExpression regex;
    QCollator collator;
    std::vector<FSEntry> fileEntryVec, dirEntryVec;
    const FSEntry defaultEntry{};
    QString mDirectoryPath;
    DirectoryWatcher* watcher = nullptr;
    SortingMode mSortingMode = SORT_NAME;
    FileListSource mListSource = SOURCE_DIRECTORY;
    // 替代 static const QString emptyString - 线程安全且性能更好
    QString mEmptyString;

    void readSettings();
    void loadEntryList(const QString &directoryPath, bool recursive);
    bool path_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool path_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    bool name_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool name_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    bool date_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool date_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    auto compareFunction() -> CompareFunction;
    bool size_entry_compare(const FSEntry &e1, const FSEntry &e2) const;
    bool size_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const;
    void startFileWatcher(const QString &directoryPath);
    void stopFileWatcher();
    void addEntriesFromDirectory(std::vector<FSEntry> &entryVec, const QString &directoryPath);
    void addEntriesFromDirectoryRecursive(std::vector<FSEntry> &entryVec, const QString &directoryPath);

    // 内联优化：范围检查函数移至头文件
    inline bool checkFileRange(int index) const {
        return index >= 0 && index < static_cast<int>(fileEntryVec.size());
    }

    inline bool checkDirRange(int index) const {
        return index >= 0 && index < static_cast<int>(dirEntryVec.size());
    }

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
