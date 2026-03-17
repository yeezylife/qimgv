#include "folderviewproxy.h"
#include <QApplication>

FolderViewProxy::FolderViewProxy(QWidget *parent)
    : QWidget(parent),
      folderView(nullptr)
{
    stateBuf.sortingMode = settings->sortingMode();
    layout.setContentsMargins(0,0,0,0);
}

void FolderViewProxy::init() {
    // 优化：进入锁前先检查，避免已初始化的情况下重复竞争锁
    if (folderView)
        return;

    qApp->processEvents(); 

    {
        QMutexLocker ml(&m);
        // 双检锁，防止 processEvents 期间的重入
        if (folderView)
            return;

        // 优化：使用 std::make_shared 减少内存分配开销
        folderView = std::make_shared<FolderView>();
        folderView->setParent(this);
    } 
    // 优化：锁在创建完成后即释放，后续 UI 连接和布局操作均在锁外进行

    layout.addWidget(folderView.get());
    this->setFocusProxy(folderView.get());
    this->setLayout(&layout);

    connect(folderView.get(), &FolderView::itemActivated, this, &FolderViewProxy::itemActivated);
    connect(folderView.get(), &FolderView::thumbnailsRequested, this, &FolderViewProxy::thumbnailsRequested);
    connect(folderView.get(), &FolderView::sortingSelected, this, &FolderViewProxy::sortingSelected);
    connect(folderView.get(), &FolderView::showFoldersChanged, this, &FolderViewProxy::showFoldersChanged);
    connect(folderView.get(), &FolderView::directorySelected, this, &FolderViewProxy::directorySelected);
    connect(folderView.get(), &FolderView::draggedOut, this, &FolderViewProxy::draggedOut);
    connect(folderView.get(), &FolderView::copyUrlsRequested, this, &FolderViewProxy::copyUrlsRequested);
    connect(folderView.get(), &FolderView::moveUrlsRequested, this, &FolderViewProxy::moveUrlsRequested);
    connect(folderView.get(), &FolderView::droppedInto, this, &FolderViewProxy::droppedInto);
    connect(folderView.get(), &FolderView::draggedOver, this, &FolderViewProxy::draggedOver);

    folderView->show();

    // 应用缓冲状态
    if(!stateBuf.directory.isEmpty())
        folderView->setDirectoryPath(stateBuf.directory);
    folderView->onFullscreenModeChanged(stateBuf.fullscreenMode);
    folderView->populate(stateBuf.itemCount);
    folderView->select(stateBuf.selection);

    qApp->processEvents();
    folderView->focusOnSelection();
    folderView->onSortingChanged(stateBuf.sortingMode);
}

void FolderViewProxy::populate(int count) {
    std::shared_ptr<FolderView> view;
    {
        QMutexLocker ml(&m);
        stateBuf.itemCount = count;
        // 优化：通过拷贝 shared_ptr 延长对象生命周期，从而在锁外执行耗时的 populate
        view = folderView;
    }

    if (view) {
        view->populate(count);
    } else {
        stateBuf.selection.clear();
    }
}

void FolderViewProxy::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    if(folderView) {
        folderView->setThumbnail(pos, thumb);
    }
}

void FolderViewProxy::select(QList<int> indices) {
    if(folderView) {
        folderView->select(indices);
    } else {
        stateBuf.selection = indices;
    }
}

void FolderViewProxy::select(int index) {
    if(folderView) {
        folderView->select(index);
    } else {
        stateBuf.selection.clear();
        stateBuf.selection << index;
    }
}

QList<int> FolderViewProxy::selection() {
    if(folderView) {
        return folderView->selection();
    }
    return stateBuf.selection;
}

void FolderViewProxy::focusOn(int index) {
    if(folderView) {
        folderView->focusOn(index);
    }
}

void FolderViewProxy::focusOnSelection() {
    if(folderView) {
        folderView->focusOnSelection();
    }
}

void FolderViewProxy::setDirectoryPath(QString path) {
    if(folderView) {
        folderView->setDirectoryPath(path);
    } else {
        stateBuf.directory = path;
    }
}

void FolderViewProxy::insertItem(int index) {
    if(folderView) {
        folderView->insertItem(index);
    } else {
        stateBuf.itemCount++;
    }
}

void FolderViewProxy::removeItem(int index) {
    if(folderView) {
        folderView->removeItem(index);
    } else {
        stateBuf.itemCount--;
        stateBuf.selection.removeAll(index);
        for(int i=0; i < stateBuf.selection.count(); i++) {
            if(stateBuf.selection[i] > index)
                stateBuf.selection[i]--;
        }
        if(!stateBuf.selection.count())
            stateBuf.selection << ((index >= stateBuf.itemCount) ? stateBuf.itemCount - 1 : index);
    }
}

void FolderViewProxy::reloadItem(int index) {
    if(folderView)
        folderView->reloadItem(index);
}

void FolderViewProxy::setDragHover(int index) {
    if(folderView)
        folderView->setDragHover(index);
}

void FolderViewProxy::addItem() {
    if(folderView) {
        folderView->addItem();
    } else {
        stateBuf.itemCount++;
    }
}

void FolderViewProxy::onFullscreenModeChanged(bool mode) {
    if(folderView) {
        folderView->onFullscreenModeChanged(mode);
    } else {
        stateBuf.fullscreenMode = mode;
    }
}

void FolderViewProxy::onSortingChanged(SortingMode mode) {
    if(folderView) {
        folderView->onSortingChanged(mode);
    } else {
        stateBuf.sortingMode = mode;
    }
}

void FolderViewProxy::showEvent(QShowEvent *event) {
    init();
    QWidget::showEvent(event);
}