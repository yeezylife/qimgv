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
    switch (mSortingMode) {
        case SortingMode::SORT_NAME:
            return &DirectoryManager::name_entry_compare;
        case SortingMode::SORT_NAME_DESC:
            return &DirectoryManager::name_entry_compare_reverse;
        case SortingMode::SORT_TIME:
            return &DirectoryManager::date_entry_compare;
        case SortingMode::SORT_TIME_DESC:
            return &DirectoryManager::date_entry_compare_reverse;
        case SortingMode::SORT_SIZE:
            return &DirectoryManager::size_entry_compare;
        case SortingMode::SORT_SIZE_DESC:
            return &DirectoryManager::size_entry_compare_reverse;
        default:
            return &DirectoryManager::name_entry_compare;
    }
}

void DirectoryManager::startFileWatcher(const QString &directoryPath) {
    if(directoryPath.isEmpty()) return;
    bool isFirstTime = !watcher;
    if(isFirstTime) {
        watcher = DirectoryWatcher::newInstance();
        connect(watcher, &DirectoryWatcher::fileCreated, this, &DirectoryManager::onFileAddedExternal, Qt::UniqueConnection);
        connect(watcher, &DirectoryWatcher::fileDeleted, this, &DirectoryManager::onFileRemovedExternal, Qt::UniqueConnection);
        connect(watcher, &DirectoryWatcher::fileModified, this, &DirectoryManager::onFileModifiedExternal, Qt::UniqueConnection);
        connect(watcher, &DirectoryWatcher::fileRenamed, this, &DirectoryManager::onFileRenamedExternal, Qt::UniqueConnection);
    }
    // 线程正在运行时，使用平滑路径切换（不重启线程）
    if(!isFirstTime && watcher->isObserving()) {
        watcher->requestWatchPath(directoryPath);
        return;
    }
    // 首次启动：设置路径并启动
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
    for (int i = index + 1; i < static_cast<int>(fileEntryVec.size()); ++i) {
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
        return false;
    }
    if(!std::filesystem::is_directory(pathObj)) {
        return false;
    }
    QDir dir(dirPath);
    if(!dir.isReadable()) {
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
        return false;
    }
    if(!std::filesystem::is_directory(pathObj)) {
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
    std::error_code ec;
    auto status = std::filesystem::status(pathObj, ec);
    if (ec) return false;
    return std::filesystem::is_regular_file(status);
}

bool DirectoryManager::isDir(const QString &path) const {
    std::filesystem::path pathObj(path.toStdWString());
    std::error_code ec;
    auto status = std::filesystem::status(pathObj, ec);
    if (ec) return false;
    return std::filesystem::is_directory(status);
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

    if (!dir.exists())
        return;

    QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    QDir::SortFlags sortFlags = QDir::Name | QDir::IgnoreCase;

    QFileInfoList entries = dir.entryInfoList(filters, sortFlags);

    for (const QFileInfo &fileInfo : entries) {
        QString name = fileInfo.fileName();
        QString path = fileInfo.absoluteFilePath();

        if (fileInfo.isDir()) {
            FSEntry newEntry;
            newEntry.name = name;
            newEntry.path = path;
            newEntry.isDirectory = true;
            dirEntryVec.emplace_back(std::move(newEntry));
        } else {
            match = regex.match(name);
            if (match.hasMatch()) {
                FSEntry newEntry;
                newEntry.name = name;
                newEntry.path = path;
                newEntry.isDirectory = false;

                std::error_code ec;
                std::filesystem::path p(fileInfo.filePath().toStdWString());

                newEntry.size = std::filesystem::file_size(p, ec);
                if (ec) continue;

                newEntry.modifyTime = std::filesystem::last_write_time(p, ec);
                if (ec) continue;

                entryVec.emplace_back(std::move(newEntry));
            }
        }
    }
}

void DirectoryManager::addEntriesFromDirectoryRecursive(std::vector<FSEntry> &entryVec, const QString &directoryPath) {
    QRegularExpressionMatch match;
    std::filesystem::path pathObj(directoryPath.toStdWString());

    std::error_code ec;
    fs::recursive_directory_iterator it(
        pathObj,
        fs::directory_options::skip_permission_denied,
        ec
    );

    if (ec) return;

    for (const auto& entry : it) {
        QString name = QString::fromStdWString(entry.path().filename().wstring());
        QString path = QString::fromStdWString(entry.path().wstring());

        match = regex.match(name);

        if (!entry.is_directory(ec) && !ec && match.hasMatch()) {
            FSEntry newEntry;
            newEntry.name = name;
            newEntry.path = path;
            newEntry.isDirectory = false;

            std::error_code ec2;

            newEntry.size = entry.file_size(ec2);
            if (ec2) continue;

            newEntry.modifyTime = entry.last_write_time(ec2);
            if (ec2) continue;

            entryVec.emplace_back(std::move(newEntry));
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
    emit fileAdded(filePath);
    return true;
}

void DirectoryManager::removeFileEntry(const QString &filePath) {
    if(!containsFile(filePath))
        return;
    int index = indexOfFile(filePath);
    fileEntryVec.erase(fileEntryVec.begin() + index);
    // 同步更新索引映射
    updateFileIndexAfterRemove(filePath, index);
    emit fileRemoved(filePath, index);
}

void DirectoryManager::updateFileEntry(const QString &filePath) {
    if(!containsFile(filePath))
        return;
    FSEntry newEntry(filePath);
    int index = indexOfFile(filePath);
    if(fileEntryVec.at(index).modifyTime != newEntry.modifyTime)
        fileEntryVec.at(index) = newEntry;
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

    int oldIndex = indexOfFile(oldFilePath.value);
    int replaceIndex = containsFile(newFilePath) ? indexOfFile(newFilePath) : -1;

    // 优化：记录 emit 所需的索引，在 vector 操作前保存
    int emitOldIndex = oldIndex;
    // replaceIndex 在 erase 后可能需要调整，但最终 newIndex 由 insert_sorted 决定

    // 先删除 replace（如存在且位置在 oldIndex 之前，避免 oldIndex 变化）
    if(replaceIndex != -1) {
        if(replaceIndex < oldIndex) {
            fileEntryVec.erase(fileEntryVec.begin() + replaceIndex);
            oldIndex--;
        } else if(replaceIndex > oldIndex) {
            fileEntryVec.erase(fileEntryVec.begin() + replaceIndex);
        }
        // replaceIndex == oldIndex 不可能发生（同一路径）
    }

    // 删除旧位置
    fileEntryVec.erase(fileEntryVec.begin() + oldIndex);

    // 插入新条目
    std::filesystem::path pathObj(newFilePath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    FSEntry newEntry(FilePath(newFilePath), FileName(newFileName), stdEntry.file_size(), stdEntry.last_write_time(), stdEntry.is_directory());

    auto cmpFn = compareFunction();
    auto it = insert_sorted(fileEntryVec, newEntry, [this, cmpFn](const FSEntry& a, const FSEntry& b) {
        return (this->*cmpFn)(a, b);
    });

    int newIndex = static_cast<int>(it - fileEntryVec.begin());

    // 性能优化：单次 rebuild 替代多次增量更新（3×O(n) → 1×O(n)）
    rebuildFileIndexMap();

    emit fileRenamed(oldFilePath.value, emitOldIndex, newFilePath, newIndex);
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
    emit dirRemoved(dirPath, index);
}

void DirectoryManager::renameDirEntry(const DirPath& oldDirPath, const DirName& newDirName) {
    if (!containsDir(oldDirPath.value))
        return;

    QFileInfo fi(oldDirPath.value);
    QString newDirPath = fi.absolutePath() + "/" + newDirName.value;

    int oldIndex = indexOfDir(oldDirPath.value);
    int replaceIndex = containsDir(newDirPath) ? indexOfDir(newDirPath) : -1;

    // 记录 emit 所需的索引
    int emitOldIndex = oldIndex;

    // 先删除 replace（如存在且位置在 oldIndex 之前）
    if(replaceIndex != -1) {
        if(replaceIndex < oldIndex) {
            dirEntryVec.erase(dirEntryVec.begin() + replaceIndex);
            oldIndex--;
        } else if(replaceIndex > oldIndex) {
            dirEntryVec.erase(dirEntryVec.begin() + replaceIndex);
        }
    }

    // 删除旧路径
    dirEntryVec.erase(dirEntryVec.begin() + oldIndex);

    // 构造新条目并插入
    std::filesystem::path pathObj(newDirPath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    FSEntry newEntry;
    newEntry.name = newDirName.value;
    newEntry.path = newDirPath;
    newEntry.isDirectory = true;

    auto cmpFn = compareFunction();
    auto it = insert_sorted(dirEntryVec, newEntry, [this, cmpFn](const FSEntry& a, const FSEntry& b) {
        return (this->*cmpFn)(a, b);
    });

    int newIndex = static_cast<int>(it - dirEntryVec.begin());

    // 性能优化：单次 rebuild 替代多次增量更新
    rebuildDirIndexMap();

    emit dirRenamed(oldDirPath.value, emitOldIndex, newDirPath, newIndex);
}

QStringList DirectoryManager::fileList() const {
    QStringList list;
    list.reserve(static_cast<int>(fileEntryVec.size()));
    for(auto const& value : fileEntryVec)
        list << value.path;
    return list;
}

void DirectoryManager::onFileRemovedExternal(const QString &fileName) {
    if(mIgnoreWatcherEvents)
        return;
    QString fullPath = watcher->watchPath() + "/" + fileName;
    removeDirEntry(fullPath);
    removeFileEntry(fullPath);
}

void DirectoryManager::onFileAddedExternal(const QString &fileName) {
    if(mIgnoreWatcherEvents)
        return;
    QString fullPath = watcher->watchPath() + "/" + fileName;
    if(isDir(fullPath))
        insertDirEntry(fullPath);
    else
        insertFileEntry(fullPath);
}

void DirectoryManager::onFileRenamedExternal(const QString &oldName, const QString &newName) {
    if(mIgnoreWatcherEvents)
        return;
    QString oldPath = watcher->watchPath() + "/" + oldName;
    QString newPath = watcher->watchPath() + "/" + newName;
    if(isDir(newPath))
        renameDirEntry(DirPath(oldPath), DirName(newName));
    else
        renameFileEntry(FilePath(oldPath), FileName(newName));
}

void DirectoryManager::onFileModifiedExternal(const QString &fileName) {
    if(mIgnoreWatcherEvents)
        return;
    updateFileEntry(watcher->watchPath() + "/" + fileName);
}
