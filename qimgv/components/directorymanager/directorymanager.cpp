#include "directorymanager.h"

namespace fs = std::filesystem;

DirectoryManager::DirectoryManager() {
    regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    collator.setNumericMode(true);

    readSettings();
    setSortingMode(settings->sortingMode());
    connect(settings, &Settings::settingsChanged, this, &DirectoryManager::readSettings);
}

template< typename T, typename Pred >
typename std::vector<T>::iterator
insert_sorted(std::vector<T> & vec, T const& item, Pred pred) {
    return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, pred), item);
}

bool DirectoryManager::path_entry_compare(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.path, e2.path) < 0;
};

bool DirectoryManager::path_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.path, e2.path) > 0;
};

bool DirectoryManager::name_entry_compare(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.name, e2.name) < 0;
};

bool DirectoryManager::name_entry_compare_reverse(const FSEntry &e1, const FSEntry &e2) const {
    return collator.compare(e1.name, e2.name) > 0;
};

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

CompareFunction DirectoryManager::compareFunction() {
    CompareFunction cmpFn = &DirectoryManager::path_entry_compare;
    if(mSortingMode == SortingMode::SORT_NAME_DESC)
        cmpFn = &DirectoryManager::path_entry_compare_reverse;
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
    if(directoryPath == "")
        return;
    if(!watcher)
        watcher = DirectoryWatcher::newInstance();

    connect(watcher, &DirectoryWatcher::fileCreated,  this, &DirectoryManager::onFileAddedExternal,    Qt::UniqueConnection);
    connect(watcher, &DirectoryWatcher::fileDeleted,  this, &DirectoryManager::onFileRemovedExternal,  Qt::UniqueConnection);
    connect(watcher, &DirectoryWatcher::fileModified, this, &DirectoryManager::onFileModifiedExternal, Qt::UniqueConnection);
    connect(watcher, &DirectoryWatcher::fileRenamed,  this, &DirectoryManager::onFileRenamedExternal,  Qt::UniqueConnection);

    watcher->setWatchPath(directoryPath);
    watcher->observe();
}

void DirectoryManager::stopFileWatcher() {
    if(!watcher)
        return;

    disconnect(watcher, &DirectoryWatcher::fileCreated,  this, &DirectoryManager::onFileAddedExternal);
    disconnect(watcher, &DirectoryWatcher::fileDeleted,  this, &DirectoryManager::onFileRemovedExternal);
    disconnect(watcher, &DirectoryWatcher::fileModified, this, &DirectoryManager::onFileModifiedExternal);
    disconnect(watcher, &DirectoryWatcher::fileRenamed,  this, &DirectoryManager::onFileRenamedExternal);

    watcher->stopObserving();
    
    if (watcher->isObserving()) {
        qDebug() << "[DirectoryManager] Warning: File watcher did not stop properly";
    }
}

void DirectoryManager::readSettings() {
    regex.setPattern(settings->supportedFormatsRegex());
}

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

QString DirectoryManager::directoryPath() const {
    if(mListSource == SOURCE_DIRECTORY || mListSource == SOURCE_DIRECTORY_RECURSIVE)
        return mDirectoryPath;
    else
        return "";
}

int DirectoryManager::indexOfFile(const QString &filePath) const {
    auto item = find_if(fileEntryVec.begin(), fileEntryVec.end(), [filePath](const FSEntry& e) {
        return e.path == filePath;
    });
    if(item != fileEntryVec.end())
        return static_cast<int>(distance(fileEntryVec.begin(), item));
    return -1;
}

int DirectoryManager::indexOfDir(const QString &dirPath) const {
    auto item = find_if(dirEntryVec.begin(), dirEntryVec.end(), [dirPath](const FSEntry& e) {
        return e.path == dirPath;
    });
    if(item != dirEntryVec.end())
        return static_cast<int>(distance(dirEntryVec.begin(), item));
    return -1;
}

const QString &DirectoryManager::filePathAt(int index) const {
    static const QString emptyString;
    return checkFileRange(index) ? fileEntryVec.at(index).path : emptyString;
}

const QString &DirectoryManager::fileNameAt(int index) const {
    static const QString emptyString;
    return checkFileRange(index) ? fileEntryVec.at(index).name : emptyString;
}

const QString &DirectoryManager::dirPathAt(int index) const {
    static const QString emptyString;
    return checkDirRange(index) ? dirEntryVec.at(index).path : emptyString;
}

const QString &DirectoryManager::dirNameAt(int index) const {
    static const QString emptyString;
    return checkDirRange(index) ? dirEntryVec.at(index).name : emptyString;
}

const QString &DirectoryManager::firstFile() const {
    static const QString emptyString;
    return fileEntryVec.empty() ? emptyString : fileEntryVec.front().path;
}

const QString &DirectoryManager::lastFile() const {
    static const QString emptyString;
    return fileEntryVec.empty() ? emptyString : fileEntryVec.back().path;
}

const QString &DirectoryManager::prevOfFile(const QString &filePath) const {
    static const QString emptyString;
    int currentIndex = indexOfFile(filePath);
    return (currentIndex > 0) ? fileEntryVec.at(currentIndex - 1).path : emptyString;
}

const QString &DirectoryManager::nextOfFile(const QString &filePath) const {
    static const QString emptyString;
    int currentIndex = indexOfFile(filePath);
    return (currentIndex >= 0 && currentIndex < static_cast<int>(fileEntryVec.size()) - 1) 
            ? fileEntryVec.at(currentIndex + 1).path : emptyString;
}

const QString &DirectoryManager::prevOfDir(const QString &dirPath) const {
    static const QString emptyString;
    int currentIndex = indexOfDir(dirPath);
    return (currentIndex > 0) ? dirEntryVec.at(currentIndex - 1).path : emptyString;
}

const QString &DirectoryManager::nextOfDir(const QString &dirPath) const {
    static const QString emptyString;
    int currentIndex = indexOfDir(dirPath);
    return (currentIndex >= 0 && currentIndex < static_cast<int>(dirEntryVec.size()) - 1) 
            ? dirEntryVec.at(currentIndex + 1).path : emptyString;
}

bool DirectoryManager::checkFileRange(int index) const {
    return index >= 0 && index < (int)fileEntryVec.size();
}

bool DirectoryManager::checkDirRange(int index) const {
    return index >= 0 && index < (int)dirEntryVec.size();
}

unsigned long DirectoryManager::totalCount() const {
    return fileCount() + dirCount();
}

unsigned long DirectoryManager::fileCount() const {
    return fileEntryVec.size();
}

unsigned long DirectoryManager::dirCount() const {
    return dirEntryVec.size();
}

const FSEntry &DirectoryManager::fileEntryAt(int index) const {
    if(checkFileRange(index))
        return fileEntryVec.at(index);
    else
        return defaultEntry;
}

QDateTime DirectoryManager::lastModified(const QString &filePath) const {
    QFileInfo info;
    if(containsFile(filePath))
        info.setFile(filePath);
    return info.lastModified();
}

inline
bool DirectoryManager::isSupportedFile(const QString &path) const {
    return ( isFile(path) && regex.match(path).hasMatch() );
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

bool DirectoryManager::isEmpty() const {
    return fileEntryVec.empty();
}

bool DirectoryManager::containsFile(const QString &filePath) const {
    return (std::find(fileEntryVec.begin(), fileEntryVec.end(), filePath) != fileEntryVec.end());
}

bool DirectoryManager::containsDir(const QString &dirPath) const {
    return (std::find(dirEntryVec.begin(), dirEntryVec.end(), dirPath) != dirEntryVec.end());
}

void DirectoryManager::loadEntryList(const QString &directoryPath, bool recursive) {
    dirEntryVec.clear();
    fileEntryVec.clear();
    if(recursive) {
        addEntriesFromDirectoryRecursive(fileEntryVec, directoryPath);
    } else {
        addEntriesFromDirectory(fileEntryVec, directoryPath);
    }
}

void DirectoryManager::addEntriesFromDirectory(std::vector<FSEntry> &entryVec, const QString &directoryPath) {
    QRegularExpressionMatch match;
    QDir dir(directoryPath);
    if (!dir.exists()) {
        return;
    }
    
    QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    if (settings->showHiddenFiles()) {
        filters |= QDir::Hidden;
    }
    
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
}

void DirectoryManager::sortFileEntryListsIncremental() {
    CompareFunction currentCompareFn = compareFunction();
    if (mLastCompareFunction == currentCompareFn && mFilesSorted && fileEntryVec.size() > 1) {
        return;
    }
    if ((mSortingMode == SORT_NAME || mSortingMode == SORT_NAME_DESC) && fileEntryVec.size() < 100) {
        std::sort(fileEntryVec.begin(), fileEntryVec.end(), std::bind(currentCompareFn, this, std::placeholders::_1, std::placeholders::_2));
    } else {
        if (fileEntryVec.size() > 1) {
            std::sort(fileEntryVec.begin(), fileEntryVec.end(), std::bind(currentCompareFn, this, std::placeholders::_1, std::placeholders::_2));
        }
    }
    mLastCompareFunction = currentCompareFn;
    mFilesSorted = true;
}

void DirectoryManager::sortDirEntryListsIncremental() {
    CompareFunction currentCompareFn = compareFunction();
    if (mLastCompareFunction == currentCompareFn && mDirsSorted && dirEntryVec.size() > 1) {
        return;
    }
    if (settings->sortFolders()) {
        std::sort(dirEntryVec.begin(), dirEntryVec.end(), std::bind(currentCompareFn, this, std::placeholders::_1, std::placeholders::_2));
    } else {
        std::sort(dirEntryVec.begin(), dirEntryVec.end(), std::bind(&DirectoryManager::path_entry_compare, this, std::placeholders::_1, std::placeholders::_2));
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

SortingMode DirectoryManager::sortingMode() const {
    return mSortingMode;
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
    
    // 使用包装类构造
    FSEntry entry(FilePath{filePath}, FileName{fileName}, stdEntry.file_size(), stdEntry.last_write_time(), stdEntry.is_directory());
    insert_sorted(fileEntryVec, entry, std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
    
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

void DirectoryManager::renameFileEntry(FilePath oldFilePath, FileName newFileName) {
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
        emit fileRemoved(newFilePath, replaceIndex);
    }
    
    int oldIndex = indexOfFile(oldFilePath.value);
    fileEntryVec.erase(fileEntryVec.begin() + oldIndex);
    
    std::filesystem::path pathObj(newFilePath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    FSEntry newEntry(FilePath{newFilePath}, FileName{newFileName}, stdEntry.file_size(), stdEntry.last_write_time(), stdEntry.is_directory());
    insert_sorted(fileEntryVec, newEntry, std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
    
    qDebug() << "fileRen" << oldFilePath.value << newFilePath;
    emit fileRenamed(oldFilePath.value, oldIndex, newFilePath, indexOfFile(newFilePath));
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
    insert_sorted(dirEntryVec, newEntry, std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
    qDebug() << "dirIns" << dirPath;
    emit dirAdded(dirPath);
    return true;
}

void DirectoryManager::removeDirEntry(const QString &dirPath) {
    if(!containsDir(dirPath))
        return;
    int index = indexOfDir(dirPath);
    dirEntryVec.erase(dirEntryVec.begin() + index);
    qDebug() << "dirRem" << dirPath;
    emit dirRemoved(dirPath, index);
}

void DirectoryManager::renameDirEntry(DirPath oldDirPath, DirName newDirName) {
    if(!containsDir(oldDirPath.value))
        return;
    QFileInfo fi(oldDirPath.value);
    QString newDirPath = fi.absolutePath() + "/" + newDirName.value;
    int oldIndex = indexOfDir(oldDirPath.value);
    dirEntryVec.erase(dirEntryVec.begin() + oldIndex);
    
    std::filesystem::path pathObj(newDirPath.toStdWString());
    std::filesystem::directory_entry stdEntry(pathObj);
    FSEntry newEntry;
    newEntry.name = newDirName.value;
    newEntry.path = newDirPath;
    newEntry.isDirectory = true;
    insert_sorted(dirEntryVec, newEntry, std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
    
    qDebug() << "dirRen" << oldDirPath.value << newDirPath;
    emit dirRenamed(oldDirPath.value, oldIndex, newDirPath, indexOfDir(newDirPath));
}

FileListSource DirectoryManager::source() const {
    return mListSource;
}

QStringList DirectoryManager::fileList() const {
    QStringList list;
    for(auto const& value : fileEntryVec)
        list << value.path;
    return list;
}

bool DirectoryManager::fileWatcherActive() {
    if(!watcher)
        return false;
    return watcher->isObserving();
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
        renameDirEntry(oldPath, newName); // 利用隐式构造
    else
        renameFileEntry(oldPath, newName); // 利用隐式构造
}

void DirectoryManager::onFileModifiedExternal(const QString &fileName) {
    updateFileEntry(watcher->watchPath() + "/" + fileName);
}