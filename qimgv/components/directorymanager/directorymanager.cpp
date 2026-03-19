#include "directorymanager.h"

namespace fs = std::filesystem;

DirectoryManager::DirectoryManager() {
    regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    
    // 关键修改：显式设置区域设置为系统默认，确保 NumericMode 生效
    collator.setLocale(QLocale::system());
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    readSettings();
    setSortingMode(settings->sortingMode());
    connect(settings, &Settings::settingsChanged, this, &DirectoryManager::readSettings);
    // mEmptyString 默认构造即为空字符串，无需额外初始化
}

template<typename T, typename Pred>
typename std::vector<T>::iterator insert_sorted(std::vector<T> &vec, T const& item, Pred pred) {
    return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, pred), item);
}

bool DirectoryManager::path_entry_compare(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.path, e2.path) < 0;
}

bool DirectoryManager::path_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.path, e2.path) > 0;
}

bool DirectoryManager::name_entry_compare(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.name, e2.name) < 0;
}

bool DirectoryManager::name_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.name, e2.name) > 0;
}

bool DirectoryManager::date_entry_compare(const FSEntry& e1, const FSEntry& e2) const {
    return e1.modifyTime < e2.modifyTime;
}

bool DirectoryManager::date_entry_compare_reverse(const FSEntry& e1, const FSEntry& e2) const {
    return e1.modifyTime > e2.modifyTime;
}

bool DirectoryManager::size_entry_compare(const FSEntry& e1, const FSEntry& e2) const {
    return e1.size < e2.size;
}

bool DirectoryManager::size_entry_compare_reverse(const FSEntry& e1, const FSEntry& e2) const {
    return e1.size > e2.size;
}

// 修复编译错误：使用尾置返回类型
auto DirectoryManager::compareFunction() -> CompareFunction {
    CompareFunction cmpFn = &DirectoryManager::name_entry_compare;
    if(mSortingMode == SortingMode::SORT_NAME_DESC)
        cmpFn = &DirectoryManager::name_entry_compare_reverse;
    if(mSortingMode == SortingMode::SORT_TIME)
        cmpFn = &DirectoryManager::date_entry_compare;
    if(mSortingMode == SortingMode::SORT_TIME_DESC)
        cmpFn = &DirectoryManager::date_entry_compare_reverse;
    if(mSortingMode == SortingMode::SORT_SIZE)
        cmpFn = &DirectoryManager::size_entry_compare;
    if(mSortingMode == SortingMode::SORT_SIZE_DESC)
        cmpFn = &DirectoryManager::size_entry_compare_reverse;
    return cmpFn;
}

void DirectoryManager::startFileWatcher(const QString &directoryPath) {
    if(directoryPath == "") return;
    if(!watcher)
        watcher = DirectoryWatcher::newInstance();
    connect(watcher, &DirectoryWatcher::fileCreated, this, &DirectoryManager::onFileAddedExternal, Qt::UniqueConnection);
    connect(watcher, &DirectoryWatcher::fileDeleted, this, &DirectoryManager::onFileRemovedExternal, Qt::UniqueConnection);
    connect(watcher, &DirectoryWatcher::fileModified, this, &DirectoryManager::onFileModifiedExternal, Qt::UniqueConnection);
    connect(watcher, &DirectoryWatcher::fileRenamed, this, &DirectoryManager::onFileRenamedExternal, Qt::UniqueConnection);
    watcher->setWatchPath(directoryPath);
    watcher->observe();
}

void DirectoryManager::stopFileWatcher() {
    if(!watcher) return;
    disconnect(watcher, &DirectoryWatcher::fileCreated, this, &DirectoryManager::onFileAddedExternal);
    disconnect(watcher, &DirectoryWatcher::fileDeleted, this, &DirectoryManager::onFileRemovedExternal);
    disconnect(watcher, &DirectoryWatcher::fileModified, this, &DirectoryManager::onFileModifiedExternal);
    disconnect(watcher, &DirectoryWatcher::fileRenamed, this, &DirectoryManager::onFileRenamedExternal);
    watcher->stopObserving();
    if (watcher->isObserving()) {
        qDebug() << "[DirectoryManager] Warning: File watcher did not stop properly";
    }
}

void DirectoryManager::readSettings() {
    regex.setPattern(settings->supportedFormatsRegex());
}

// ==================== 索引映射维护方法 ====================

void DirectoryManager::rebuildFileIndexMap() {
    mFileIndexMap.clear();
    mFileIndexMap.reserve(fileEntryVec.size());
    for(size_t i = 0; i < fileEntryVec.size(); ++i) {
        mFileIndexMap[fileEntryVec[i].path] = static_cast<int>(i);
    }
}

void DirectoryManager::rebuildDirIndexMap() {
    mDirIndexMap.clear();
    mDirIndexMap.reserve(dirEntryVec.size());
    for(size_t i = 0; i < dirEntryVec.size(); ++i) {
        mDirIndexMap[dirEntryVec[i].path] = static_cast<int>(i);
    }
}

void DirectoryManager::updateFileIndexAfterInsert(const QString &path, int index) {
    // 插入新元素
    mFileIndexMap[path] = index;

    // 更新 index 之后的所有元素（+1 已经体现在 vector 中，这里只需重写索引）
    for (int i = index; i < size; ++i)
        mFileIndexMap[fileEntryVec[i].path] = i;
    }
}

void DirectoryManager::updateFileIndexAfterRemove(const QString &path, int index) {
    // 删除元素
    mFileIndexMap.erase(path);

    // 更新 index 之后的所有元素（vector 已前移）
    for (int i = index; i < static_cast<int>(fileEntryVec.size()); ++i) {
        mFileIndexMap[fileEntryVec[i].path] = i;
    }
}

void DirectoryManager::updateDirIndexAfterInsert(const QString &path, int index) {
    mDirIndexMap[path] = index;

    for (int i = index + 1; i < static_cast<int>(dirEntryVec.size()); ++i) {
        mDirIndexMap[dirEntryVec[i].path] = i;
    }
}

void DirectoryManager::updateDirIndexAfterRemove(const QString &path, int index) {
    mDirIndexMap.erase(path);

    for (int i = index; i < static_cast<int>(dirEntryVec.size()); ++i) {
        mDirIndexMap[dirEntryVec[i].path] = i;
    }
}

// ==================== 核心功能方法 ====================

bool DirectoryManager::setDirectory(const QString &dirPath) {
    if(dirPath.isEmpty()) {
        return false;
    }
    std::filesystem::path pathObj(dirPath.toStdWString());
    if(!std::filesystem::exists(pathObj)) {
        qDebug() << "[DirectoryManager] Error - path does not exist.";
        return false;
    }
    if(!std::filesystem::is_directory(pathObj)) {
        qDebug() << "[DirectoryManager] Error - path is not a directory.";
        return false;
    }
    QDir dir(dirPath);
    if(!dir.isReadable()) {
        qDebug() << "[DirectoryManager] Error - cannot read directory.";
        return false;
    }
    mListSource = SOURCE_DIRECTORY;
    mDirectoryPath = dirPath;
    loadEntryList(dirPath, false);
    sortEntryLists();
    emit loaded(dirPath);
    startFileWatcher(dirPath);
    return true;
}

bool DirectoryManager::setDirectoryRecursive(const QString &dirPath) {
    if(dirPath.isEmpty()) {
        return false;
    }
    std::filesystem::path pathObj(dirPath.toStdWString());
    if(!std::filesystem::exists(pathObj)) {
        qDebug() << "[DirectoryManager] Error - path does not exist.";
        return false;
    }
    if(!std::filesystem::is_directory(pathObj)) {
        qDebug() << "[DirectoryManager] Error - path is not a directory.";
        return false;
    }
    stopFileWatcher();
    mListSource = SOURCE_DIRECTORY_RECURSIVE;
    mDirectoryPath = dirPath;
    loadEntryList(dirPath, true);
    sortEntryLists();
    emit loaded(dirPath);
    return true;
}

// 性能优化：O(n) → O(1)
int DirectoryManager::indexOfFile(const QString &filePath) const {
    auto it = mFileIndexMap.find(filePath);
    return (it != mFileIndexMap.end()) ? it->second : -1;
}

// 性能优化：O(n) → O(1)
int DirectoryManager::indexOfDir(const QString &dirPath) const {
    auto it = mDirIndexMap.find(dirPath);
    return (it != mDirIndexMap.end()) ? it->second : -1;
}

const QString &DirectoryManager::filePathAt(int index) const {
    return checkFileRange(index) ? fileEntryVec.at(index).path : mEmptyString;
}

const QString &DirectoryManager::fileNameAt(int index) const {
    return checkFileRange(index) ? fileEntryVec.at(index).name : mEmptyString;
}

const QString &DirectoryManager::dirPathAt(int index) const {
    return checkDirRange(index) ? dirEntryVec.at(index).path : mEmptyString;
}

const QString &DirectoryManager::dirNameAt(int index) const {
    return checkDirRange(index) ? dirEntryVec.at(index).name : mEmptyString;
}

const QString &DirectoryManager::firstFile() const {
    return fileEntryVec.empty() ? mEmptyString : fileEntryVec.front().path;
}

const QString &DirectoryManager::lastFile() const {
    return fileEntryVec.empty() ? mEmptyString : fileEntryVec.back().path;
}

// 性能优化：indexOfFile 现在是 O(1)
const QString &DirectoryManager::prevOfFile(const QString &filePath) const {
    int currentIndex = indexOfFile(filePath);
    return (currentIndex > 0) ? fileEntryVec.at(currentIndex - 1).path : mEmptyString;
}

const QString &DirectoryManager::nextOfFile(const QString &filePath) const {
    int currentIndex = indexOfFile(filePath);
    return (currentIndex >= 0 && currentIndex < static_cast<int>(fileEntryVec.size()) - 1) ? fileEntryVec.at(currentIndex + 1).path : mEmptyString;
}

const QString &DirectoryManager::prevOfDir(const QString &dirPath) const {
    int currentIndex = indexOfDir(dirPath);
    return (currentIndex > 0) ? dirEntryVec.at(currentIndex - 1).path : mEmptyString;
}

const QString &DirectoryManager::nextOfDir(const QString &dirPath) const {
    int currentIndex = indexOfDir(dirPath);
    return (currentIndex >= 0 && currentIndex < static_cast<int>(dirEntryVec.size()) - 1) ? dirEntryVec.at(currentIndex + 1).path : mEmptyString;
}

const FSEntry &DirectoryManager::fileEntryAt(int index) const {
    if(checkFileRange(index))
        return fileEntryVec.at(index);
    return defaultEntry;
}

QDateTime DirectoryManager::lastModified(const QString &filePath) const {
    QFileInfo info;
    if(containsFile(filePath))
        info.setFile(filePath);
    return info.lastModified();
}

inline bool DirectoryManager::isSupportedFile(const QString &path) const {
    return (isFile(path) && regex.match(path).hasMatch());
}

bool DirectoryManager::isFile(const QString &path) const {
    std::filesystem::path pathObj(path.toStdWString());
    if(!std::filesystem::exists(pathObj))
        return false;
    if(!std::filesystem::is_regular_file(pathObj))
        return false;
    return true;
}

bool DirectoryManager::isDir(const QString &path) const {
    std::filesystem::path pathObj(path.toStdWString());
    if(!std::filesystem::exists(pathObj))
        return false;
    if(!std::filesystem::is_directory(pathObj))
        return false;
    return true;
}

void DirectoryManager::loadEntryList(const QString &directoryPath, bool recursive) {
    dirEntryVec.clear();
    fileEntryVec.clear();
    mFileIndexMap.clear();
    mDirIndexMap.clear();
    // 预分配容量，减少内存重分配
    fileEntryVec.reserve(1000);
    dirEntryVec.reserve(100);

    if(recursive) {
        addEntriesFromDirectoryRecursive(fileEntryVec, directoryPath);
    } else {
        addEntriesFromDirectory(fileEntryVec, directoryPath);
    }
    // 加载后构建索引映射
    rebuildFileIndexMap();
    rebuildDirIndexMap();
}

void DirectoryManager::addEntriesFromDirectory(std::vector<FSEntry> &entryVec, const QString &directoryPath) {
    QRegularExpressionMatch match;
    QDir dir(directoryPath);
    if (!dir.exists()) {
        return;
    }
    
    // 优化：去除隐藏文件判断逻辑，明确指定所需的过滤器
    QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    
    QDir::SortFlags sortFlags = QDir::Name | QDir::IgnoreCase;
    QFileInfoList entries = dir.entryInfoList(filters, sortFlags);
    for (const QFileInfo &fileInfo : entries) {
        QString name = fileInfo.fileName();
        QString path = fileInfo.absoluteFilePath();
        if (fileInfo.isDir()) {
            FSEntry newEntry;
            try {
                newEntry.name = name;
                newEntry.path = path;
                newEntry.isDirectory = true;
                dirEntryVec.emplace_back(newEntry);
            } catch (...) {
                qDebug() << "[DirectoryManager] Error creating directory entry for:" << path;
                continue;
            }
        } else {
            match = regex.match(name);
            if (match.hasMatch()) {
                FSEntry newEntry;
                try {
                    newEntry.name = name;
                    newEntry.path = path;
                    newEntry.isDirectory = false;
                    newEntry.size = fileInfo.size();
                    newEntry.modifyTime = std::filesystem::last_write_time(std::filesystem::path(fileInfo.filePath().toStdWString()));
                    entryVec.emplace_back(newEntry);
                } catch (...) {
                    qDebug() << "[DirectoryManager] Error creating file entry for:" << path;
                    continue;
                }
            }
        }
    }
}

void DirectoryManager::addEntriesFromDirectoryRecursive(std::vector<FSEntry> &entryVec, const QString &directoryPath) {
    QRegularExpressionMatch match;
    std::filesystem::path pathObj(directoryPath.toStdWString());
    for(const auto & entry : fs::recursive_directory_iterator(pathObj)) {
        QString name = QString::fromStdWString(entry.path().filename().wstring());
        QString path = QString::fromStdWString(entry.path().wstring());
        match = regex.match(name);
        if(!entry.is_directory() && match.hasMatch()) {
            FSEntry newEntry;
            try {
                newEntry.name = name;
                newEntry.path = path;
                newEntry.isDirectory = false;
                newEntry.size = entry.file_size();
                newEntry.modifyTime = entry.last_write_time();
            } catch (const std::filesystem::filesystem_error &err) {
                qDebug() << "[DirectoryManager]" << err.what();
                continue;
            }
            entryVec.emplace_back(newEntry);
        }
    }
}

void DirectoryManager::sortEntryLists() {
    sortFileEntryListsIncremental();
    sortDirEntryListsIncremental();
    // 排序后索引变化，需要重建映射
    rebuildFileIndexMap();
    rebuildDirIndexMap();
}

void DirectoryManager::sortFileEntryListsIncremental() {
    CompareFunction currentCompareFn = compareFunction();
    if (mLastCompareFunction == currentCompareFn && mFilesSorted && fileEntryVec.size() > 1) {
        return;
    }
    // 使用 C++20 ranges::sort
    if (fileEntryVec.size() > 1) {
        std::ranges::sort(fileEntryVec, [this, currentCompareFn](const FSEntry& a, const FSEntry& b) {
            return (this->*currentCompareFn)(a, b);
        });
    }
    mLastCompareFunction = currentCompareFn;
    mFilesSorted = true;
}

void DirectoryManager::sortDirEntryListsIncremental() {
    CompareFunction currentCompareFn = compareFunction();
    if (mLastCompareFunction == currentCompareFn && mDirsSorted && dirEntryVec.size() > 1) {
        return;
    }
    // 使用 C++20 ranges::sort
    if (dirEntryVec.size() > 1) {
        if (settings->sortFolders()) {
            std::ranges::sort(dirEntryVec, [this, currentCompareFn](const FSEntry& a, const FSEntry& b) {
                return (this->*currentCompareFn)(a, b);
            });
        } else {
            std::ranges::sort(dirEntryVec, [this](const FSEntry& a, const FSEntry& b) {
                return (this->* &DirectoryManager::path_entry_compare)(a, b);
            });
        }
    }
    mLastCompareFunction = currentCompareFn;
    mDirsSorted = true;
}

void DirectoryManager::setSortingMode(SortingMode mode) {
    if(mode != mSortingMode) {
        mSortingMode = mode;
        if(fileEntryVec.size() > 1 || dirEntryVec.size() > 1) {
            sortEntryLists();
            emit sortingChanged();
        }
    }
}

bool DirectoryManager::insertFileEntry(const QString &filePath) {
    if(!isSupportedFile(filePath))
        return false;
    return forceInsertFileEntry(filePath);
}

bool DirectoryManager::forceInsertFileEntry(const QString &filePath) {
    if(!this->isFile(filePath) || containsFile(filePath))
        return false;
    std::filesystem::path pathObj(filePath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    QString fileName = QString::fromStdWString(stdEntry.path().filename().wstring());
    // 使用包装类构造 FSEntry
    FSEntry entry(FilePath(filePath), FileName(fileName), stdEntry.file_size(), stdEntry.last_write_time(), stdEntry.is_directory());

    // 优化：使用 lambda 替代 std::bind
    auto cmpFn = compareFunction();
    auto it = insert_sorted(fileEntryVec, entry, [this, cmpFn](const FSEntry& a, const FSEntry& b) {
        return (this->*cmpFn)(a, b);
    });

    int index = static_cast<int>(it - fileEntryVec.begin());
    // 同步更新索引映射
    updateFileIndexAfterInsert(filePath, index);
    if(!directoryPath().isEmpty()) {
        qDebug() << "fileIns" << filePath << directoryPath();
        emit fileAdded(filePath);
    }
    return true;
}

void DirectoryManager::removeFileEntry(const QString &filePath) {
    if(!containsFile(filePath))
        return;
    int index = indexOfFile(filePath);
    fileEntryVec.erase(fileEntryVec.begin() + index);
    // 同步更新索引映射
    updateFileIndexAfterRemove(filePath, index);
    qDebug() << "fileRem" << filePath;
    emit fileRemoved(filePath, index);
}

void DirectoryManager::updateFileEntry(const QString &filePath) {
    if(!containsFile(filePath))
        return;
    FSEntry newEntry(filePath);
    int index = indexOfFile(filePath);
    if(fileEntryVec.at(index).modifyTime != newEntry.modifyTime)
        fileEntryVec.at(index) = newEntry;
    qDebug() << "fileMod" << filePath;
    emit fileModified(filePath);
}

void DirectoryManager::renameFileEntry(const FilePath& oldFilePath, const FileName& newFileName) {
    // 显式使用 .value
    QFileInfo fi(oldFilePath.value);
    QString newFilePath = fi.absolutePath() + "/" + newFileName.value;
    if(!containsFile(oldFilePath.value)) {
        if(containsFile(newFilePath))
            updateFileEntry(newFilePath);
        else
            insertFileEntry(newFilePath);
        return;
    }
    if(!isSupportedFile(newFilePath)) {
        removeFileEntry(oldFilePath.value);
        return;
    }
    if(containsFile(newFilePath)) {
        int replaceIndex = indexOfFile(newFilePath);
        fileEntryVec.erase(fileEntryVec.begin() + replaceIndex);
        updateFileIndexAfterRemove(newFilePath, replaceIndex);  // ⭐关键补上
        emit fileRemoved(newFilePath, replaceIndex);
    }

    int oldIndex = indexOfFile(oldFilePath.value);
    fileEntryVec.erase(fileEntryVec.begin() + oldIndex);
    updateFileIndexAfterRemove(oldFilePath.value, oldIndex);  // ⭐必须
    std::filesystem::path pathObj(newFilePath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    FSEntry newEntry(FilePath(newFilePath), FileName(newFileName), stdEntry.file_size(), stdEntry.last_write_time(), stdEntry.is_directory());

    // 优化：使用 lambda 替代 std::bind
    auto cmpFn = compareFunction();
    auto it = insert_sorted(fileEntryVec, newEntry, [this, cmpFn](const FSEntry& a, const FSEntry& b) {
        return (this->*cmpFn)(a, b);
    });

    int newIndex = static_cast<int>(it - fileEntryVec.begin());
    // 此处理论上不用再同步更新索引映射，先注释掉，有问题加回来
    //rebuildFileIndexMap();
    qDebug() << "fileRen" << oldFilePath.value << newFilePath;
    emit fileRenamed(oldFilePath.value, oldIndex, newFilePath, newIndex);
}

bool DirectoryManager::insertDirEntry(const QString &dirPath) {
    if(containsDir(dirPath))
        return false;
    std::filesystem::path pathObj(dirPath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    QString dirName = QString::fromStdWString(stdEntry.path().filename().wstring());
    FSEntry newEntry;
    newEntry.name = dirName;
    newEntry.path = dirPath;
    newEntry.isDirectory = true;

    // 优化：使用 lambda 替代 std::bind
    auto cmpFn = compareFunction();
    auto it = insert_sorted(dirEntryVec, newEntry, [this, cmpFn](const FSEntry& a, const FSEntry& b) {
        return (this->*cmpFn)(a, b);
    });

    int index = static_cast<int>(it - dirEntryVec.begin());
    // 同步更新索引映射
    updateDirIndexAfterInsert(dirPath, index);
    qDebug() << "dirIns" << dirPath;
    emit dirAdded(dirPath);
    return true;
}

void DirectoryManager::removeDirEntry(const QString &dirPath) {
    if(!containsDir(dirPath))
        return;
    int index = indexOfDir(dirPath);
    dirEntryVec.erase(dirEntryVec.begin() + index);
    // 同步更新索引映射
    updateDirIndexAfterRemove(dirPath, index);
    qDebug() << "dirRem" << dirPath;
    emit dirRemoved(dirPath, index);
}

void DirectoryManager::renameDirEntry(const DirPath& oldDirPath, const DirName& newDirName) {
    if(!containsDir(oldDirPath.value))
        return;
    QFileInfo fi(oldDirPath.value);
    QString newDirPath = fi.absolutePath() + "/" + newDirName.value;
    int oldIndex = indexOfDir(oldDirPath.value);
    dirEntryVec.erase(dirEntryVec.begin() + oldIndex);
    updateDirIndexAfterRemove(oldDirPath.value, oldIndex);
    std::filesystem::path pathObj(newDirPath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    FSEntry newEntry;
    newEntry.name = newDirName.value;
    newEntry.path = newDirPath;
    newEntry.isDirectory = true;

    // 优化：使用 lambda 替代 std::bind
    auto cmpFn = compareFunction();
    auto it = insert_sorted(dirEntryVec, newEntry, [this, cmpFn](const FSEntry& a, const FSEntry& b) {
        return (this->*cmpFn)(a, b);
    });

    int newIndex = static_cast<int>(it - dirEntryVec.begin());
    // 同步更新索引映射
    rebuildDirIndexMap();
    qDebug() << "dirRen" << oldDirPath.value << newDirPath;
    emit dirRenamed(oldDirPath.value, oldIndex, newDirPath, newIndex);
}

QStringList DirectoryManager::fileList() const {
    QStringList list;
    list.reserve(static_cast<int>(fileEntryVec.size()));
    for(auto const& value : fileEntryVec)
        list << value.path;
    return list;
}

void DirectoryManager::onFileRemovedExternal(const QString &fileName) {
    QString fullPath = watcher->watchPath() + "/" + fileName;
    removeDirEntry(fullPath);
    removeFileEntry(fullPath);
}

void DirectoryManager::onFileAddedExternal(const QString &fileName) {
    QString fullPath = watcher->watchPath() + "/" + fileName;
    if(isDir(fullPath))
        insertDirEntry(fullPath);
    else
        insertFileEntry(fullPath);
}

void DirectoryManager::onFileRenamedExternal(const QString &oldName, const QString &newName) {
    QString oldPath = watcher->watchPath() + "/" + oldName;
    QString newPath = watcher->watchPath() + "/" + newName;
    if(isDir(newPath))
        renameDirEntry(DirPath(oldPath), DirName(newName));
    else
        renameFileEntry(FilePath(oldPath), FileName(newName));
}

void DirectoryManager::onFileModifiedExternal(const QString &fileName) {
    updateFileEntry(watcher->watchPath() + "/" + fileName);
}
