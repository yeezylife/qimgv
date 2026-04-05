#include "directorypresenter.h"
#include "sourcecontainers/thumbnail.h"

DirectoryPresenter::DirectoryPresenter(QObject *parent) : QObject(parent), mShowDirs(false) {
    connect(&thumbnailer, &Thumbnailer::thumbnailReady, this, &DirectoryPresenter::onThumbnailReady);
}

void DirectoryPresenter::unsetModel() {
    if (!model)
        return;
    disconnect(model.get(), &DirectoryModel::fileRemoved,  this, &DirectoryPresenter::onFileRemoved);
    disconnect(model.get(), &DirectoryModel::fileAdded,    this, &DirectoryPresenter::onFileAdded);
    disconnect(model.get(), &DirectoryModel::fileRenamed,  this, &DirectoryPresenter::onFileRenamed);
    disconnect(model.get(), &DirectoryModel::fileModified, this, &DirectoryPresenter::onFileModified);
    disconnect(model.get(), &DirectoryModel::dirRemoved,   this, &DirectoryPresenter::onDirRemoved);
    disconnect(model.get(), &DirectoryModel::dirAdded,     this, &DirectoryPresenter::onDirAdded);
    disconnect(model.get(), &DirectoryModel::dirRenamed,   this, &DirectoryPresenter::onDirRenamed);
    model = nullptr;
}

QObject *DirectoryPresenter::viewAsObject() {
    if (!viewObject && view)
        viewObject = dynamic_cast<QObject *>(view.get());
    return viewObject;
}

void DirectoryPresenter::setView(const std::shared_ptr<IDirectoryView> &_view) {
    if(view)
        return;
    view = _view;
    viewObject = dynamic_cast<QObject *>(view.get());
    if(model)
        view->populate(mShowDirs ? qMin(static_cast<int>(model->totalCount()), INT_MAX) : qMin(static_cast<int>(model->fileCount()), INT_MAX));
    connect(viewObject, SIGNAL(itemActivated(int)),
            this, SLOT(onItemActivated(int)));
    connect(viewObject, SIGNAL(thumbnailsRequested(QList<int>, int, bool, bool)),
            this, SLOT(generateThumbnails(QList<int>, int, bool, bool)));
    connect(viewObject, SIGNAL(draggedOut()),
            this, SLOT(onDraggedOut()));
    connect(viewObject, SIGNAL(draggedOver(int)),
            this, SLOT(onDraggedOver(int)));
    connect(viewObject, SIGNAL(droppedInto(const QMimeData*,QObject*,int)),
            this, SLOT(onDroppedInto(const QMimeData*,QObject*,int)));
}

void DirectoryPresenter::setModel(const std::shared_ptr<DirectoryModel> &newModel) {
    if(model)
        unsetModel();
    if(!newModel)
        return;
    model = newModel;
    populateView();

    // filesystem changes
    connect(model.get(), &DirectoryModel::fileRemoved,  this, &DirectoryPresenter::onFileRemoved);
    connect(model.get(), &DirectoryModel::fileAdded,    this, &DirectoryPresenter::onFileAdded);
    connect(model.get(), &DirectoryModel::fileRenamed,  this, &DirectoryPresenter::onFileRenamed);
    connect(model.get(), &DirectoryModel::fileModified, this, &DirectoryPresenter::onFileModified);
    connect(model.get(), &DirectoryModel::dirRemoved,   this, &DirectoryPresenter::onDirRemoved);
    connect(model.get(), &DirectoryModel::dirAdded,     this, &DirectoryPresenter::onDirAdded);
    connect(model.get(), &DirectoryModel::dirRenamed,   this, &DirectoryPresenter::onDirRenamed);
}

void DirectoryPresenter::reloadModel() {
    populateView();
}

void DirectoryPresenter::populateView() {
    if(!model || !view)
        return;
    view->populate(mShowDirs ? qMin(static_cast<int>(model->totalCount()), INT_MAX) : qMin(static_cast<int>(model->fileCount()), INT_MAX));
    selectAndFocus(0);
}

void DirectoryPresenter::disconnectView() {
   // todo
}

//------------------------------------------------------------------------------

// 辅助函数：将文件索引转换为视图中的绝对索引
int DirectoryPresenter::fileIndexToViewIndex(int fileIndex) const {
    return mShowDirs ? fileIndex + model->dirCount() : fileIndex;
}

void DirectoryPresenter::onFileRemoved(const QString &filePath, int index) {
    Q_UNUSED(filePath)
    if(!view)
        return;
    view->removeItem(fileIndexToViewIndex(index));
}

void DirectoryPresenter::onFileRenamed(const QString &fromPath, int indexFrom, const QString &toPath, int indexTo) {
    Q_UNUSED(fromPath)
    Q_UNUSED(toPath)
    if(!view)
        return;
    
    // 转换为视图索引
    int viewIndexFrom = fileIndexToViewIndex(indexFrom);
    int viewIndexTo = fileIndexToViewIndex(indexTo);
    
    // removeItem 后，如果 viewIndexFrom < viewIndexTo，插入位置需要减1
    auto oldSelection = view->selection();
    view->removeItem(viewIndexFrom);
    if(viewIndexFrom < viewIndexTo)
        viewIndexTo--;
    view->insertItem(viewIndexTo);
    
    // re-select if needed
    if(oldSelection.contains(viewIndexFrom)) {
        if(oldSelection.count() == 1) {
            view->select(viewIndexTo);
            view->focusOn(viewIndexTo);
        } else if(oldSelection.count() > 1) {
            view->select(view->selection() << viewIndexTo);
        }
    }
}

void DirectoryPresenter::onFileAdded(const QString &filePath) {
    if(!view)
        return;
    int index = model->indexOfFile(filePath);
    view->insertItem(fileIndexToViewIndex(index));
}

void DirectoryPresenter::onFileModified(const QString &filePath) {
    if(!view)
        return;
    int index = model->indexOfFile(filePath);
    view->reloadItem(fileIndexToViewIndex(index));
}

void DirectoryPresenter::onDirRemoved(const QString &dirPath, int index) {
    Q_UNUSED(dirPath)
    if(!view || !mShowDirs)
        return;
    view->removeItem(index);
}

void DirectoryPresenter::onDirRenamed(const QString &fromPath, int indexFrom, const QString &toPath, int indexTo) {
    Q_UNUSED(fromPath)
    Q_UNUSED(toPath)
    if(!view || !mShowDirs)
        return;
    auto oldSelection = view->selection();
    view->removeItem(indexFrom);
    view->insertItem(indexTo);
    // re-select if needed
    if(oldSelection.contains(indexFrom)) {
        if(oldSelection.count() == 1) {
            view->select(indexTo);
            view->focusOn(indexTo);
        } else if(oldSelection.count() > 1) {
            view->select(view->selection() << indexTo);
        }
    }
}

void DirectoryPresenter::onDirAdded(const QString &dirPath) {
    if(!view || !mShowDirs)
        return;
    int index = model->indexOfDir(dirPath);
    view->insertItem(index);
}

bool DirectoryPresenter::showDirs() {
    return mShowDirs;
}

void DirectoryPresenter::setShowDirs(bool mode) {
    if(mode == mShowDirs)
        return;
    mShowDirs = mode;
    populateView();
}

QList<QString> DirectoryPresenter::selectedPaths() const {
    QList<QString> paths;
    if(!view || !model)
        return paths;
        
    const auto& selection = view->selection();
    const int dirCount = model->dirCount();
    
    if(mShowDirs) {
        // 预分配容量以提高性能
        paths.reserve(selection.size());
        
        for(auto i : selection) {
            if(i < dirCount)
                paths << model->dirPathAt(i);
            else
                paths << model->filePathAt(i - dirCount);
        }
    } else {
        // 预分配容量以提高性能
        paths.reserve(selection.size());
        
        for(auto i : selection) {
            paths << model->filePathAt(i);
        }
    }
    return paths;
}

void DirectoryPresenter::generateThumbnails(const QList<int> &indexes, int size, bool crop, bool force) {
    // 完全跳过缩略图生成
    if(!settings->useThumbnailCache()) {
        return;
    }
    
    if(!view || !model)
        return;
    
    thumbnailer.clearTasks();
    
    // 如果不显示目录，直接批量处理文件
    if(!mShowDirs) {
        for(int i : indexes)
            thumbnailer.getThumbnailAsync(model->filePathAt(i), size, crop, force);
        return;
    }
    
    // 如果缓存的缩略图大小不匹配，则重新创建
    if(!mCachedDirPixmap || mCachedDirSize != size) {
        QSvgRenderer svgRenderer;
        svgRenderer.load(QString(":/res/icons/common/other/folder32-scalable.svg"));
        int factor = qRound((size * 0.90) / static_cast<qreal>(svgRenderer.defaultSize().width()));
        QPixmap pix(svgRenderer.defaultSize() * factor);
        pix.fill(Qt::transparent);
        QPainter pixPainter(&pix);
        svgRenderer.render(&pixPainter);
        pixPainter.end();
        
        ImageLib::recolor(pix, settings->colorScheme().icons);
        mCachedDirPixmap = std::make_shared<const QPixmap>(std::move(pix));
        mCachedDirSize = size;
    }
    
    const int dirCount = model->dirCount();
    
    // 批量处理索引
    for(int i : indexes) {
        if(i < dirCount) {
            // 使用缓存的目录图标（共享指针，避免拷贝）
            std::shared_ptr<Thumbnail> thumb = std::make_shared<Thumbnail>(
                model->dirNameAt(i), size, "Folder", mCachedDirPixmap);
            view->setThumbnail(i, thumb);
        } else {
            QString path = model->filePathAt(i - dirCount);
            thumbnailer.getThumbnailAsync(path, size, crop, force);
        }
    }
}

void DirectoryPresenter::onThumbnailReady(const std::shared_ptr<Thumbnail> &thumb, const QString &filePath) {
    if(!view || !model)
        return;
    int index = model->indexOfFile(filePath);
    if(index == -1)
        return;
    view->setThumbnail(mShowDirs ? model->dirCount() + index : index, thumb);
}

void DirectoryPresenter::onItemActivated(int absoluteIndex) {
    if(!model)
        return;
    if(!mShowDirs) {
        emit fileActivated(model->filePathAt(absoluteIndex));
        return;
    }
    if(absoluteIndex < model->dirCount())
        emit dirActivated(model->dirPathAt(absoluteIndex));
    else
        emit fileActivated(model->filePathAt(absoluteIndex - model->dirCount()));
}

void DirectoryPresenter::onDraggedOut() {
    emit draggedOut(selectedPaths());
}

void DirectoryPresenter::onDraggedOver(int index) {
    if(!model || view->selection().contains(index))
        return;
    if(showDirs() && index < model->dirCount())
        view->setDragHover(index);

}

void DirectoryPresenter::onDroppedInto(const QMimeData *data, QObject *source, int targetIndex) {
    if(!data->hasUrls() || model->source() != SOURCE_DIRECTORY)
        return;

    // ignore drops into selected / current folder when we are the source of dropEvent
    if(source && (view->selection().contains(targetIndex) || targetIndex == -1))
        return;
        
    // ignore drops into a file
    // todo: drop into a current dir when target is a file
    if(showDirs() && targetIndex >= model->dirCount())
        return;

    // convert urls to qstrings using range-based for loop
    QStringList pathList;
    const QList<QUrl> urlList = data->urls();
    pathList.reserve(urlList.size()); // 预分配容量
    
    for(const QUrl &url : urlList) {
        pathList.append(url.toLocalFile());
    }

    // get target dir path
    QString destDir;
    if(showDirs() && targetIndex < model->dirCount()) {
        destDir = model->dirPathAt(targetIndex);
    }
    
    if(destDir.isEmpty()) { // fallback to the current dir
        destDir = model->directoryPath();
    }
    
    pathList.removeAll(destDir); // remove target dir from source list

    // pass to core
    emit droppedInto(pathList, destDir);
}

void DirectoryPresenter::selectAndFocus(const QString &path) {
    if(!model || !view || path.isEmpty())
        return;
        
    if(model->containsDir(path) && showDirs()) {
        int dirIndex = model->indexOfDir(path);
        view->select(dirIndex);
        view->focusOn(dirIndex);
    } else if(model->containsFile(path)) {
        int fileIndex = fileIndexToViewIndex(model->indexOfFile(path));
        view->select(fileIndex);
        view->focusOn(fileIndex);
    }
}

void DirectoryPresenter::selectAndFocus(int absoluteIndex) {
    if(!model || !view)
        return;
    view->select(absoluteIndex);
    view->focusOn(absoluteIndex);
}
