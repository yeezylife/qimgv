#include "directorymodel.h"
#include "sourcecontainers/fsentry.h"

DirectoryModel::DirectoryModel(QObject *parent)
    : QObject(parent), fileListSource(SOURCE_DIRECTORY) {
    scaler = new Scaler(&cache);
    // 设置缓存大小限制为50个项目
    cache.setMaxCacheSize(50);
    connect(&dirManager, &DirectoryManager::fileRemoved, this, &DirectoryModel::onFileRemoved);
    connect(&dirManager, &DirectoryManager::fileAdded, this, &DirectoryModel::onFileAdded);
    connect(&dirManager, &DirectoryManager::fileRenamed, this, &DirectoryModel::onFileRenamed);
    connect(&dirManager, &DirectoryManager::fileModified, this, &DirectoryModel::onFileModified);
    connect(&dirManager, &DirectoryManager::dirRemoved, this, &DirectoryModel::dirRemoved);
    connect(&dirManager, &DirectoryManager::dirAdded, this, &DirectoryModel::dirAdded);
    connect(&dirManager, &DirectoryManager::dirRenamed, this, &DirectoryModel::dirRenamed);
    connect(&dirManager, &DirectoryManager::loaded, this, &DirectoryModel::loaded);
    connect(&dirManager, &DirectoryManager::sortingChanged, this, &DirectoryModel::onSortingChanged);
    connect(&loader, &Loader::loadFinished, this, &DirectoryModel::onImageReady);
    connect(&loader, &Loader::loadFailed, this, &DirectoryModel::loadFailed);
}

DirectoryModel::~DirectoryModel() {
    loader.clearTasks();
    delete scaler;
}

size_t DirectoryModel::totalCount() const {
    return dirManager.totalCount();
}

int DirectoryModel::fileCount() const {
    return static_cast<int>(dirManager.fileCount());
}

int DirectoryModel::dirCount() const {
    return static_cast<int>(dirManager.dirCount());
}

int DirectoryModel::indexOfFile(const QString &filePath) const {
    return dirManager.indexOfFile(filePath);
}

int DirectoryModel::indexOfDir(const QString &filePath) const {
    return dirManager.indexOfDir(filePath);
}

SortingMode DirectoryModel::sortingMode() const {
    return dirManager.sortingMode();
}

const FSEntry &DirectoryModel::fileEntryAt(int index) const {
    return dirManager.fileEntryAt(index);
}

const QString &DirectoryModel::fileNameAt(int index) const {
    return dirManager.fileNameAt(index);
}

const QString &DirectoryModel::filePathAt(int index) const {
    return dirManager.filePathAt(index);
}

const QString &DirectoryModel::dirNameAt(int index) const {
    return dirManager.dirNameAt(index);
}

const QString &DirectoryModel::dirPathAt(int index) const {
    return dirManager.dirPathAt(index);
}

bool DirectoryModel::autoRefresh() {
    return dirManager.fileWatcherActive();
}

FileListSource DirectoryModel::source() {
    return dirManager.source();
}

const QString DirectoryModel::directoryPath() const {
    return dirManager.directoryPath();
}

bool DirectoryModel::containsFile(const QString &filePath) const {
    return dirManager.containsFile(filePath);
}

bool DirectoryModel::containsDir(const QString &dirPath) const {
    return dirManager.containsDir(dirPath);
}

bool DirectoryModel::isEmpty() const {
    return dirManager.isEmpty();
}

const QString &DirectoryModel::firstFile() const {
    return dirManager.firstFile();
}

const QString &DirectoryModel::lastFile() const {
    return dirManager.lastFile();
}

const QString &DirectoryModel::nextOf(const QString &filePath) const {
    return dirManager.nextOfFile(filePath);
}

const QString &DirectoryModel::prevOf(const QString &filePath) const {
    return dirManager.prevOfFile(filePath);
}

const QDateTime DirectoryModel::lastModified(const QString &filePath) const {
    return dirManager.lastModified(filePath);
}

// -----------------------------------------------------------------------------
bool DirectoryModel::forceInsert(const QString &filePath) {
    return dirManager.forceInsertFileEntry(filePath);
}

void DirectoryModel::setSortingMode(SortingMode mode) {
    dirManager.setSortingMode(mode);
}

void DirectoryModel::removeFile(const QString &filePath, bool trash, FileOpResult &result) {
    if(trash)
        FileOperations::moveToTrash(filePath, result);
    else
        FileOperations::removeFile(filePath, result);
    if(result != FileOpResult::SUCCESS)
        return;
    dirManager.removeFileEntry(filePath);
    return;
}

void DirectoryModel::renameEntry(const QString &oldPath, const QString &newName, bool force, FileOpResult &result) {
    bool isDir = dirManager.isDir(oldPath);
    FileOperations::rename(oldPath, newName, force, result);

    // chew through watcher events so they wont be processed out of order
    qApp->processEvents();

    if(result != FileOpResult::SUCCESS)
        return;

    if (isDir) {
        dirManager.renameDirEntry(DirPath(oldPath),
                                  DirName(newName));
    } else {
        dirManager.renameFileEntry(FilePath(oldPath),
                                   FileName(newName));
    } 
} 

void DirectoryModel::removeDir(const QString &dirPath, bool trash, bool recursive, FileOpResult &result) {
    if(trash) {
        FileOperations::moveToTrash(dirPath, result);
    } else {
        FileOperations::removeDir(dirPath, recursive, result);
    }
    if(result != FileOpResult::SUCCESS)
        return;
    dirManager.removeDirEntry(dirPath);
    return;
}

void DirectoryModel::copyFileTo(const QString &srcFile, const QString &destDirPath, bool force, FileOpResult &result) {
    FileOperations::copyFileTo(srcFile, destDirPath, force, result);
}

void DirectoryModel::moveFileTo(const QString &srcFile, const QString &destDirPath, bool force, FileOpResult &result) {
    FileOperations::moveFileTo(srcFile, destDirPath, force, result);
    // chew through watcher events so they wont be processed out of order
    qApp->processEvents();
    if(result == FileOpResult::SUCCESS) {
        if(destDirPath != this->directoryPath())
            dirManager.removeFileEntry(srcFile);
    }
}

// -----------------------------------------------------------------------------
bool DirectoryModel::setDirectory(const QString &path) {
    cache.clear();
    return dirManager.setDirectory(path);
}

void DirectoryModel::unload(int index) {
    QString filePath = this->filePathAt(index);
    cache.remove(filePath);
}

void DirectoryModel::unload(const QString &filePath) {
    cache.remove(filePath);
}

void DirectoryModel::unloadExcept(const QString &filePath, bool keepNearby) {
    QList<QString> list;
    list << filePath;
    if(keepNearby) {
        list << prevOf(filePath);
        list << nextOf(filePath);
    }
    cache.trimTo(list);
}

bool DirectoryModel::loaderBusy() const {
    return loader.isBusy();
}

void DirectoryModel::onImageReady(const std::shared_ptr<Image> &img, const QString &path) {
    if(!img) {
        emit loadFailed(path);
        return;
    }
    cache.remove(path);
    cache.insert(img);
    emit imageReady(img, path);
}

bool DirectoryModel::saveFile(const QString &filePath) {
    return saveFile(filePath, filePath);
}

bool DirectoryModel::saveFile(const QString &filePath, const QString &destPath) {
    if(!containsFile(filePath) || !cache.contains(filePath))
        return false;
    auto img = cache.get(filePath);
    if(img->save(destPath)) {
        if(filePath == destPath) {
            // replace
            dirManager.updateFileEntry(destPath);
            emit fileModified(destPath);
        } else {
            // manually add if we are saving to the same dir
            QFileInfo fiSrc(filePath);
            QFileInfo fiDest(destPath);
            // handle same dir
            if(fiSrc.absolutePath() == fiDest.absolutePath()) {
                // overwrite
                if(!dirManager.containsFile(destPath) && dirManager.insertFileEntry(destPath))
                    emit fileModified(destPath);
            }
        }
        return true;
    }
    return false;
}

// dirManager events
void DirectoryModel::onSortingChanged() {
    emit sortingChanged(sortingMode());
}

void DirectoryModel::onFileAdded(const QString &filePath) {
    emit fileAdded(filePath);
}

void DirectoryModel::onFileModified(const QString &filePath) {
    QDateTime modTime = lastModified(filePath);
    if(modTime.isValid()) {
        auto img = cache.get(filePath);
        if(img) {
            // check if file on disk is different
            if(modTime != img->lastModified())
                reload(filePath);
        }
        emit fileModified(filePath);
    }
}

void DirectoryModel::onFileRemoved(const QString &filePath, int index) {
    unload(filePath);
    emit fileRemoved(filePath, index);
}

void DirectoryModel::onFileRenamed(const QString &fromPath, int indexFrom, const QString &toPath, int indexTo) {
    unload(fromPath);
    emit fileRenamed(fromPath, indexFrom, toPath, indexTo);
}

bool DirectoryModel::isLoaded(int index) const {
    return cache.contains(filePathAt(index));
}

bool DirectoryModel::isLoaded(const QString& filePath) const {
    return cache.contains(filePath);
}

std::shared_ptr<Image> DirectoryModel::getImageAt(int index) {
    return getImage(filePathAt(index));
}

// returns cached image
// if image is not cached, loads it in the main thread
// for async access use loadAsync(), then catch onImageReady()
std::shared_ptr<Image> DirectoryModel::getImage(const QString &filePath) {
    std::shared_ptr<Image> img = cache.get(filePath);
    if(!img)
        img = loader.load(filePath);
    return img;
}

void DirectoryModel::updateImage(const QString &filePath, const std::shared_ptr<Image> &img) {
    if(containsFile(filePath) /*& cache.contains(filePath)*/) {
        if(!cache.contains(filePath)) {
            cache.insert(img);
        } else {
            cache.insert(img);
            emit imageUpdated(filePath);
        }
    }
}

void DirectoryModel::load(const QString &filePath, bool asyncHint) {
    if(!containsFile(filePath) || loader.isLoading(filePath))
        return;
    if(!cache.contains(filePath)) {
        if(asyncHint) {
            loader.loadAsyncPriority(filePath);
        } else {
            auto img = loader.load(filePath);
            if(img) {
                cache.insert(img);
                emit imageReady(img, filePath);
            } else {
                emit loadFailed(filePath);
            }
        }
    } else {
        emit imageReady(cache.get(filePath), filePath);
    }
}

void DirectoryModel::reload(const QString &filePath) {
    if(cache.contains(filePath)) {
        cache.remove(filePath);
        dirManager.updateFileEntry(filePath);
        load(filePath, false);
    }
}

void DirectoryModel::preload(const QString &filePath) {
    if(containsFile(filePath) && !cache.contains(filePath))
        loader.loadAsync(filePath);
}

// 线程安全的辅助方法
bool DirectoryModel::isCacheFull() const {
    QMutexLocker locker(&mMutex);
    return cache.currentCacheSize() >= 30; // 可配置的缓存限制
}

void DirectoryModel::clearCache() {
    QMutexLocker locker(&mMutex);
    cache.clear();
}

int DirectoryModel::getCacheSize() const {
    QMutexLocker locker(&mMutex);
    return cache.currentCacheSize();
}