#include "directorymanager.h"
#include <QHash>
#include <iterator>

namespace fs = std::filesystem;

DirectoryManager::DirectoryManager() {
    // 关键修改：显式设置区域设置为系统默认，确保 NumericMode 生效
    collator.setLocale(QLocale::system());
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    readSettings();
    setSortingMode(settings->sortingMode());
    connect(settings, &Settings::settingsChanged, this, &DirectoryManager::readSettings);
    // 批量事件：0ms 单次定时器，合并同一轮事件循环内到达的 watcher 事件
    mEventBatchTimer.setSingleShot(true);
    mEventBatchTimer.setInterval(0);
    connect(&mEventBatchTimer, &QTimer::timeout, this, &DirectoryManager::flushPendingEvents);
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
    clearPendingEvents();
}

void DirectoryManager::readSettings() {
    for (auto &bucket : mSuffixBuckets)
        bucket.clear();
    mSuffixLongBucket.clear();

    const auto &formats = settings->supportedFormats();
    for (const auto &fmt : formats) {
        QString suffix = QString::fromLatin1(fmt).toLower();
        const int len = suffix.size();
        if (len >= 1 && len <= kMaxBuiltinSuffixLen)
            mSuffixBuckets[len - 1].push_back(std::move(suffix));
        else if (len > kMaxBuiltinSuffixLen)
            mSuffixLongBucket.push_back(std::move(suffix));
        // len == 0 不可能出现（格式名非空），跳过即可
    }
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
    // ⭐ 单次 status 同时完成存在性与类型判断，避免重复 stat
    std::error_code ec;
    std::filesystem::path pathObj(dirPath.toStdWString());
    const auto st = std::filesystem::status(pathObj, ec);
    if(ec || !std::filesystem::is_directory(st)) {
        return false;
    }
    QFileInfo dirInfo(dirPath);
    if(!dirInfo.isReadable()) {
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
    // ⭐ 单次 status 同时完成存在性与类型判断，避免重复 stat
    std::error_code ec;
    std::filesystem::path pathObj(dirPath.toStdWString());
    const auto st = std::filesystem::status(pathObj, ec);
    if(ec || !std::filesystem::is_directory(st)) {
        return false;
    }
    QFileInfo dirInfo(dirPath);
    if(!dirInfo.isReadable()) {
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

bool DirectoryManager::isSupportedSuffix(const QStringView &suffix) const {
    // ⭐ 按长度分桶匹配：不同长度的字符串必然不相等，只需与同长度桶内
    // 已统一小写的后缀做大小写不敏感比较；QStringView 全程无分配。
    // 目录扫描热路径：替代原先对全部后缀的线性扫描
    const int len = suffix.size();
    if (len <= 0)
        return false;

    const std::vector<QString> &bucket =
        (len <= kMaxBuiltinSuffixLen) ? mSuffixBuckets[len - 1] : mSuffixLongBucket;

    for (const QString &s : bucket) {
        if (suffix.compare(s, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
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
    // 重置增量排序标志，确保 directory_iterator 无序结果被正确排序
    mFilesSorted = false;
    mDirsSorted = false;
    // 清理积压的 watcher 事件，避免旧目录事件串入新列表
    clearPendingEvents();
    // 容量交由 vector 自增长：精确 reserve 需先完整遍历一遍目录，
    // 其枚举开销大于省下的少量重分配（FSEntry 为 POD 类结构，扩容仅 memcpy），
    // 故不再预分配——小目录零浪费，大目录指数扩容
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
    std::filesystem::path pathObj(directoryPath.toStdWString());
    std::error_code ec;
    fs::directory_iterator it(pathObj, fs::directory_options::skip_permission_denied, ec);
    if (ec) return;

    for (const auto& entry : it) {
        const auto &fsPath = entry.path();
        QString name = QString::fromStdWString(fsPath.filename().wstring());
        // generic_wstring(): Windows 上统一正斜杠分隔，与 Qt 全局路径约定（QFileInfo::absoluteFilePath()）一致
        QString path = QString::fromStdWString(fsPath.generic_wstring());

        if (entry.is_directory(ec) && !ec) {
            FSEntry newEntry;
            newEntry.name = name;
            newEntry.path = path;
            newEntry.isDirectory = true;
            dirEntryVec.emplace_back(std::move(newEntry));
        } else if (!ec) {
            const qsizetype dot = name.lastIndexOf(u'.');
            const bool supported = (dot > 0 && dot < name.size() - 1)
                                   && isSupportedSuffix(QStringView(name).mid(dot + 1));
            if (supported) {
                // ⭐ QFileInfo 单次 stat 取齐 size/mtime，替代 file_size+last_write_time 两次重复 stat
                if (auto newEntry = FSEntry::fromPath(path, name))
                    entryVec.emplace_back(std::move(*newEntry));
            }
        }
    }
}

void DirectoryManager::addEntriesFromDirectoryRecursive(std::vector<FSEntry> &entryVec, const QString &directoryPath) {
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
        // generic_wstring(): 顺带修复递归扫描同款反斜杠路径回归
        QString path = QString::fromStdWString(entry.path().generic_wstring());

        const qsizetype dot = name.lastIndexOf(u'.');
        const bool supported = (dot > 0 && dot < name.size() - 1)
                               && isSupportedSuffix(QStringView(name).mid(dot + 1));

        if (!entry.is_directory(ec) && !ec && supported) {
            // ⭐ QFileInfo 单次 stat 取齐 size/mtime，替代两次重复 stat
            if (auto newEntry = FSEntry::fromPath(path, name))
                entryVec.emplace_back(std::move(*newEntry));
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
    // ⭐ QFileInfo 单次 stat 取齐类型与元数据，替代 directory_entry 构造 + 后续重复 stat
    auto entry = FSEntry::fromPath(filePath);
    if(!entry || entry->isDirectory)
        return false;

    const qsizetype dot = filePath.lastIndexOf(u'.');
    if(dot < 0 || dot == filePath.size() - 1)
        return false;
    if(!isSupportedSuffix(QStringView(filePath).mid(dot + 1)))
        return false;

    return forceInsertFileEntry(filePath, *entry);
}

bool DirectoryManager::forceInsertFileEntry(const QString &filePath) {
    auto entry = FSEntry::fromPath(filePath);
    if(!entry || entry->isDirectory)
        return false;
    return forceInsertFileEntry(filePath, *entry);
}

bool DirectoryManager::forceInsertFileEntry(const QString &filePath, const FSEntry &entry) {
    if(containsFile(filePath))
        return false;
    // FSEntry 已含全部元数据（单次 stat），直接插入，无额外 stat
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
    if(fileEntryVec.at(index).modifyTime != newEntry.modifyTime) {
        fileEntryVec.at(index) = newEntry;
        emit fileModified(filePath);
    }
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
    // ⭐ QFileInfo 单次 stat 完成类型判断与元数据获取，替代 directory_entry + 重复 stat
    auto newEntryOpt = FSEntry::fromPath(newFilePath, newFileName.value);
    if(!newEntryOpt || newEntryOpt->isDirectory) {
        removeFileEntry(oldFilePath.value);
        return;
    }
    const qsizetype dot = newFilePath.lastIndexOf(u'.');
    if(dot < 0 || dot == newFilePath.size() - 1
       || !isSupportedSuffix(QStringView(newFilePath).mid(dot + 1))) {
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

    // 插入新条目（复用上面单次 stat 的元数据，无额外 stat）
    FSEntry newEntry = std::move(*newEntryOpt);

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
    // 不再构造 directory_entry 做无谓 stat，文件名直接由路径解析
    QString dirName = FSEntry::extractFileName(dirPath);
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

    PendingEvent ev;
    ev.type = PendingEvent::Type::Remove;
    ev.path = QDir(watcher->watchPath()).filePath(fileName);
    mPendingEvents.append(ev);
    mEventBatchTimer.start();
}

void DirectoryManager::onFileAddedExternal(const QString &fileName) {
    if(mIgnoreWatcherEvents)
        return;

    PendingEvent ev;
    ev.type = PendingEvent::Type::Add;
    ev.path = QDir(watcher->watchPath()).filePath(fileName);
    mPendingEvents.append(ev);
    mEventBatchTimer.start();
}

void DirectoryManager::onFileRenamedExternal(const QString &oldName, const QString &newName) {
    if(mIgnoreWatcherEvents)
        return;

    PendingEvent ev;
    ev.type = PendingEvent::Type::Rename;
    ev.path = QDir(watcher->watchPath()).filePath(oldName);
    ev.newName = newName;
    mPendingEvents.append(ev);
    mEventBatchTimer.start();
}

void DirectoryManager::onFileModifiedExternal(const QString &fileName) {
    if(mIgnoreWatcherEvents)
        return;

    PendingEvent ev;
    ev.type = PendingEvent::Type::Modify;
    ev.path = QDir(watcher->watchPath()).filePath(fileName);
    mPendingEvents.append(ev);
    mEventBatchTimer.start();
}

// ==================== 批量事件处理 ====================

void DirectoryManager::clearPendingEvents() {
    mEventBatchTimer.stop();
    mPendingEvents.clear();
}

void DirectoryManager::flushPendingEvents() {
    if(mIgnoreWatcherEvents) {
        clearPendingEvents();
        return;
    }

    // 快照并立即清空，处理期间新到达的事件进入下一批
    QVector<PendingEvent> events = std::move(mPendingEvents);
    mPendingEvents.clear();

    // ---- 按真实到达顺序合并 ----
    // add/remove：同一路径由最后一次到达的 op 决定（跨队列时序不再丢失）
    QHash<QString, bool> finalOp;          // true=add, false=remove
    // rename：同旧路径只保留最后一次；renameSeq 记录其序号用于按序输出
    QHash<QString, QString> renameFinal;
    QHash<QString, int> renameSeq;
    QHash<QString, int> firstRenameSeq;    // 旧路径首次被 rename 的序号
    QHash<QString, int> addSeq;            // add 事件序号（判定"新建后随即重命名"）
    QSet<QString> modifySet;
    int seq = 0;
    for(const auto &ev : events) {
        switch(ev.type) {
        case PendingEvent::Type::Add:
            addSeq.insert(ev.path, seq);
            finalOp.insert(ev.path, true);
            break;
        case PendingEvent::Type::Remove:
            finalOp.insert(ev.path, false);
            break;
        case PendingEvent::Type::Rename:
            if(!firstRenameSeq.contains(ev.path))
                firstRenameSeq.insert(ev.path, seq);
            renameFinal.insert(ev.path, ev.newName);
            renameSeq.insert(ev.path, seq);
            break;
        case PendingEvent::Type::Modify:
            modifySet.insert(ev.path);
            break;
        }
        ++seq;
    }

    // rename 按到达顺序输出（同旧路径取最后一次），保证链式 rename 可顺序套用
    QVector<QPair<QString, QString>> renames;
    {
        QVector<QPair<QString, int>> ordered;
        ordered.reserve(renameSeq.size());
        for(auto it = renameSeq.constBegin(); it != renameSeq.constEnd(); ++it)
            ordered.append(QPair<QString, int>(it.key(), it.value()));
        std::sort(ordered.begin(), ordered.end(),
                  [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                      return a.second < b.second;
                  });
        renames.reserve(ordered.size());
        for(const auto &o : ordered)
            renames.append(QPair<QString, QString>(o.first, renameFinal.value(o.first)));
    }

    // "新建后随即重命名"：add 早于该路径首次 rename → 该 add 被消费，不再单独入列表
    QSet<QString> consumedAdds;
    for(auto it = addSeq.constBegin(); it != addSeq.constEnd(); ++it) {
        auto f = firstRenameSeq.constFind(it.key());
        if(f != firstRenameSeq.constEnd() && it.value() < f.value())
            consumedAdds.insert(it.key());
    }

    QVector<QString> removes;
    QVector<QString> adds;
    for(auto it = finalOp.constBegin(); it != finalOp.constEnd(); ++it) {
        if(it.value()) {
            if(!consumedAdds.contains(it.key()))
                adds.append(it.key());
        } else {
            removes.append(it.key());
        }
    }

    // 处理顺序：删除 → 重命名 → 新增 → 修改
    // （先删后改名："删除 b + 把 a 改名为 b"必须按磁盘时序先删 b，rename 覆盖语义才正确）
    processPendingRemovals(removes);
    processPendingRenames(renames);
    processPendingAdditions(adds);
    processPendingModifications(modifySet);
}

void DirectoryManager::processPendingRenames(const QVector<QPair<QString, QString>> &renames) {
    if(renames.isEmpty() || !watcher)
        return;

    const QString base = watcher->watchPath();
    for(const auto &r : renames) {
        const QString &oldPath = r.first;
        const QString &newName = r.second;

        // ⭐ 优先用已有索引判断（避免文件系统时序问题）
        if(containsDir(oldPath)) {
            renameDirEntry(DirPath(oldPath), DirName(newName));
            continue;
        }
        if(containsFile(oldPath)) {
            renameFileEntry(FilePath(oldPath), FileName(newName));
            continue;
        }

        // ⭐ fallback：旧条目不在列表中（本批内"新建后随即重命名"，add 已被消费；
        // 或 watcher 丢事件 / 初始不同步）→ 直接把新路径插入列表；
        // 目标已在列表中则改为刷新元数据（覆盖"temp 重命名覆盖已跟踪文件"的原子替换场景）
        const QString newPath = QDir(base).filePath(newName);
        if(isDir(newPath))
            insertDirEntry(newPath);
        else if(isFile(newPath)) {
            if(containsFile(newPath))
                updateFileEntry(newPath);
            else
                insertFileEntry(newPath);
        }
    }
}

void DirectoryManager::processPendingRemovals(const QVector<QString> &removes) {
    if(removes.isEmpty())
        return;

    // 去重
    QVector<QString> paths;
    {
        QSet<QString> seen;
        paths.reserve(removes.size());
        for(const auto &p : removes) {
            if(!seen.contains(p)) {
                seen.insert(p);
                paths.append(p);
            }
        }
    }

    // 目录删除：批量快照索引 → 一次性擦除 → 单次重建 → 从高到低 emit
    {
        QVector<QPair<QString, int>> toRemove;
        for(const auto &p : paths)
            if(containsDir(p))
                toRemove.append(QPair<QString, int>(p, indexOfDir(p)));
        if(!toRemove.isEmpty()) {
            QSet<QString> removalSet;
            for(const auto &r : toRemove) removalSet.insert(r.first);
            std::vector<FSEntry> survivors;
            survivors.reserve(dirEntryVec.size() - toRemove.size());
            for(auto &e : dirEntryVec)
                if(!removalSet.contains(e.path))
                    survivors.push_back(std::move(e));
            dirEntryVec = std::move(survivors);
            rebuildDirIndexMap();
            // 从高到低 emit，保证视图连续删除时索引始终有效
            std::sort(toRemove.begin(), toRemove.end(),
                      [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                          return a.second > b.second;
                      });
            for(const auto &r : toRemove)
                emit dirRemoved(r.first, r.second);
        }
    }

    // 文件删除：同理（此时 dirCount 已更新，视图索引按当前目录数计算）
    {
        // ⭐ 原子替换（如 QSaveFile 提交：删除+重建）同样会触发 REMOVED 事件，
        // 但文件仍在磁盘上 → 实为"替换"而非删除：刷新元数据即可，不删除条目，
        // 避免 Core::onFileRemoved 误判为真删除而跳到下一张图片
        QVector<QString> kept;
        QVector<QPair<QString, int>> toRemove;
        for(const auto &p : paths) {
            if(!containsFile(p))
                continue;
            if(QFileInfo::exists(p)) {
                kept.append(p);
                continue;
            }
            toRemove.append(QPair<QString, int>(p, indexOfFile(p)));
        }
        if(!kept.isEmpty())
            for(const auto &p : kept)
                updateFileEntry(p);
        if(!toRemove.isEmpty()) {
            QSet<QString> removalSet;
            for(const auto &r : toRemove) removalSet.insert(r.first);
            std::vector<FSEntry> survivors;
            survivors.reserve(fileEntryVec.size() - toRemove.size());
            for(auto &e : fileEntryVec)
                if(!removalSet.contains(e.path))
                    survivors.push_back(std::move(e));
            fileEntryVec = std::move(survivors);
            rebuildFileIndexMap();
            // 从高到低 emit，保证视图连续删除时索引始终有效
            std::sort(toRemove.begin(), toRemove.end(),
                      [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                          return a.second > b.second;
                      });
            for(const auto &r : toRemove)
                emit fileRemoved(r.first, r.second);
        }
    }
}

void DirectoryManager::processPendingAdditions(const QVector<QString> &adds) {
    if(adds.isEmpty())
        return;

    // 去重；已存在的条目改为刷新元数据（覆盖"删除后重建"等场景）
    QVector<QString> paths;
    {
        QSet<QString> seen;
        paths.reserve(adds.size());
        for(const auto &p : adds) {
            if(seen.contains(p)) continue;
            seen.insert(p);
            if(containsFile(p) || containsDir(p)) {
                if(containsFile(p))
                    updateFileEntry(p);
                continue;
            }
            paths.append(p);
        }
    }

    // ⭐ QFileInfo 单次 stat 取齐类型与元数据，替代 directory_entry 构造 + 重复 stat
    std::vector<FSEntry> newFiles;
    std::vector<FSEntry> newDirs;
    for(const auto &p : paths) {
        auto e = FSEntry::fromPath(p);
        if(!e)
            continue;
        if(e->isDirectory) {
            newDirs.push_back(std::move(*e));
        } else {
            // ⭐ 用 basename 取后缀，与 addEntriesFromDirectory 保持一致，
            // 避免监视目录路径自身含点时误判
            const QString name = e->name;
            const qsizetype dot = name.lastIndexOf(u'.');
            if(dot <= 0 || dot == name.size() - 1) continue;
            if(!isSupportedSuffix(QStringView(name).mid(dot + 1))) continue;
            newFiles.push_back(std::move(*e));
        }
    }

    // 文件：批量追加 → 单次排序 → 单次重建 → emit
    if(!newFiles.empty()) {
        QVector<QString> addedPaths;
        addedPaths.reserve(static_cast<qsizetype>(newFiles.size()));
        for(const auto &e : newFiles) addedPaths.append(e.path);
        fileEntryVec.insert(fileEntryVec.end(),
                            std::make_move_iterator(newFiles.begin()),
                            std::make_move_iterator(newFiles.end()));
        mFilesSorted = false;
        sortFileEntryListsIncremental();
        rebuildFileIndexMap();
        for(const auto &p : addedPaths) emit fileAdded(p);
    }
    if(!newDirs.empty()) {
        QVector<QString> addedPaths;
        addedPaths.reserve(static_cast<qsizetype>(newDirs.size()));
        for(const auto &e : newDirs) addedPaths.append(e.path);
        dirEntryVec.insert(dirEntryVec.end(),
                           std::make_move_iterator(newDirs.begin()),
                           std::make_move_iterator(newDirs.end()));
        mDirsSorted = false;
        sortDirEntryListsIncremental();
        rebuildDirIndexMap();
        for(const auto &p : addedPaths) emit dirAdded(p);
    }
}

void DirectoryManager::processPendingModifications(const QSet<QString> &modifies) {
    for(const auto &p : modifies) {
        // ⭐ 只在已存在时更新，避免无意义 filesystem 调用
        if(containsFile(p))
            updateFileEntry(p);
    }
}
