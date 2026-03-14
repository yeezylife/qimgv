#include "foldergridview.h"

// TODO: create a base class for this and the one on panel

FolderGridView::FolderGridView(QWidget *parent)
    : ThumbnailView(Qt::Vertical, parent),
      shiftedCol(-1)
{
    offscreenPreloadArea = 2300;

    this->setAcceptDrops(true);

    this->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
    this->scene.setBackgroundBrush(settings->colorScheme().folderview);
    this->setCacheMode(QGraphicsView::CacheBackground);

    // turn this off until [multi]selection is implemented
    setDrawScrollbarIndicator(false);
    setSelectMode(ACTIVATE_BY_DOUBLECLICK);

    connect(settings, &Settings::settingsChanged, [this]() {
        this->scene.setBackgroundBrush(settings->colorScheme().folderview);
    });

    setupLayout();
    connect(this, &ThumbnailView::itemActivated,
            this, &FolderGridView::onitemSelected);
}

void FolderGridView::dropEvent(QDropEvent *event) {
    event->accept();
    ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
    int index = -1;
    if(item) {
        index = static_cast<int>(thumbnails.indexOf(item));
        item->setDropHovered(false);
    }
    emit droppedInto(event->mimeData(), event->source(), index);
}

void FolderGridView::dragEnterEvent(QDragEnterEvent *event) {
    event->accept();
}

void FolderGridView::dragMoveEvent(QDragMoveEvent *event) {
    event->accept();
    ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
    int index = -1;
    if(item)
        index = static_cast<int>(thumbnails.indexOf(item));
    // unselect previous
    if(index != lastDragTarget && checkRange(lastDragTarget))
        thumbnails.at(lastDragTarget)->setDropHovered(false);
    emit draggedOver(index);
    lastDragTarget = index;
}

void FolderGridView::dragLeaveEvent(QDragLeaveEvent *event) {
    event->accept();
    if(lastDragTarget < 0 || lastDragTarget >= thumbnails.count())
        return;
    thumbnails.at(lastDragTarget)->setDropHovered(false);
}

void FolderGridView::setDragHover(int index) {
    if(!checkRange(index))
        return;
    auto item = thumbnails.at(index);
    item->setDropHovered(true);
}

void FolderGridView::onitemSelected() {
    shiftedCol = -1;
}

void FolderGridView::updateScrollbarIndicator() {
    if(!thumbnails.count() || !selection().count())
        return;
    ThumbnailWidget *thumb = thumbnails.at(lastSelected());
    qreal itemCenter = (thumb->pos().y() + (thumb->height() / 2)) / scene.height();
    int yPos = qRound(scrollBar->height() * itemCenter - indicatorSize);
    indicator = QRect(2, yPos, scrollBar->width() - 4, indicatorSize);
}

// probably unneeded
void FolderGridView::show() {
    ThumbnailView::show();
    setFocus();
}

// probably unneeded
void FolderGridView::hide() {
    ThumbnailView::hide();
    clearFocus();
}

void FolderGridView::setShowLabels(bool mode) {
    ThumbnailStyle style = mode ? THUMB_NORMAL : THUMB_SIMPLE;
    for(int i = 0; i < thumbnails.count(); i++)
        thumbnails.at(i)->setThumbStyle(style);
    updateLayout();
    fitSceneToContents();
    focusOnSelection();
}

void FolderGridView::focusOnSelection() {
    if(!thumbnails.count() || lastSelected() == -1)
        return;
    ThumbnailWidget *thumb = thumbnails.at(lastSelected());
    ensureVisible(thumb, 0, 0);
}

void FolderGridView::selectAll() {
    // 使用QBitArray高效实现全选
    selectionBits.fill(true, thumbnails.count());
    lastSelectedIndex = static_cast<int>(thumbnails.count() - 1);
    updateSelectionVisuals();
}

void FolderGridView::selectAbove() {
    if(!thumbnails.count() || lastSelected() == -1 || flowLayout->sameRow(0, lastSelected()))
        return;
    int newIndex;
    newIndex = flowLayout->itemAbove(lastSelected());
    if(shiftedCol >= 0) {
        int diff = shiftedCol - flowLayout->columnOf(lastSelected());
        newIndex += diff;
        shiftedCol = -1;
    }
    if(!checkRange(newIndex))
        newIndex = 0;
    if(rangeSelection)
        addSelectionRange(newIndex);
    else
        select(newIndex);
    scrollToCurrent();
}

void FolderGridView::selectBelow() {
    if(!thumbnails.count() || lastSelected() == -1 || flowLayout->sameRow(lastSelected(), static_cast<int>(thumbnails.count() - 1)))
        return;
    shiftedCol = -1;
    int newIndex = flowLayout->itemBelow(lastSelected());
    if(!checkRange(newIndex))
        newIndex = static_cast<int>(thumbnails.count() - 1);
    if(flowLayout->columnOf(newIndex) != flowLayout->columnOf(lastSelected()))
        shiftedCol = flowLayout->columnOf(lastSelected());
    if(rangeSelection)
        addSelectionRange(newIndex);
    else
        select(newIndex);
    scrollToCurrent();
}

void FolderGridView::selectNext() {
    if(!thumbnails.count() || lastSelected() == thumbnails.count() - 1)
        return;
    if(!rangeSelection && lastSelected() == thumbnails.count() - 1) {
        select(lastSelected());
        return;
    }
    shiftedCol = -1;
    int newIndex = lastSelected() + 1;
    if(!checkRange(newIndex))
        newIndex = static_cast<int>(thumbnails.count() - 1);
    if(rangeSelection)
        addSelectionRange(newIndex);
    else
        select(newIndex);
    scrollToCurrent();
}

void FolderGridView::selectPrev() {
    if(!thumbnails.count() || lastSelected() == 0)
        return;
    shiftedCol = -1;
    int newIndex = lastSelected() - 1;
    if(!checkRange(newIndex))
        newIndex = 0;
    if(rangeSelection)
        addSelectionRange(newIndex);
    else
        select(newIndex);
    scrollToCurrent();
}

void FolderGridView::pageUp() {
    if(!thumbnails.count() || lastSelected() == -1 || flowLayout->sameRow(0, lastSelected()))
        return;
    int newIndex = lastSelected();
    int tmp;
    // 4 rows up
    for(int i = 0; i < 4; i++) {
        tmp = flowLayout->itemAbove(newIndex);
        if(checkRange(tmp))
            newIndex = tmp;
    }
    if(shiftedCol >= 0) {
        int diff = shiftedCol - flowLayout->columnOf(newIndex);
        newIndex += diff;
        shiftedCol = -1;
    }
    if(rangeSelection)
        addSelectionRange(newIndex);
    else
        select(newIndex);
    scrollToCurrent();
}

void FolderGridView::pageDown() {
    if(!thumbnails.count() || lastSelected() == -1 || flowLayout->sameRow(lastSelected(), static_cast<int>(thumbnails.count() - 1)))
        return;
    shiftedCol = -1;
    int newIndex = lastSelected();
    int tmp;
    // 4 rows down
    for(int i = 0; i < 4; i++) {
        tmp = flowLayout->itemBelow(newIndex);
        if(checkRange(tmp))
            newIndex = tmp;
    }
    if(flowLayout->columnOf(newIndex) != flowLayout->columnOf(lastSelected()))
        shiftedCol = flowLayout->columnOf(lastSelected());
    if(rangeSelection)
        addSelectionRange(newIndex);
    else
        select(newIndex);
    scrollToCurrent();
}

void FolderGridView::selectFirst() {
    if(!thumbnails.count())
        return;
    shiftedCol = -1;
    if(rangeSelection)
        addSelectionRange(0);
    else
        select(0);
    scrollToCurrent();
}

void FolderGridView::selectLast() {
    if(!thumbnails.count())
        return;
    shiftedCol = -1;
    if(rangeSelection)
        addSelectionRange(static_cast<int>(thumbnails.count() - 1));
    else
        select(static_cast<int>(thumbnails.count() - 1));
    scrollToCurrent();
}

void FolderGridView::scrollToCurrent() {
    scrollToItem(lastSelected());
}

// same as scrollToItem minus the animation
void FolderGridView::focusOn(int index) {
    if(!checkRange(index))
        return;
    ThumbnailWidget *thumb = thumbnails.at(index);
    ensureVisible(thumb, 0, 0);
    loadVisibleThumbnailsDelayed();
}

void FolderGridView::setupLayout() {
    this->setAlignment(Qt::AlignHCenter);

    flowLayout = new FlowLayout();
    flowLayout->setContentsMargins(9,6,9,0);
    setFrameShape(QFrame::NoFrame);
    scene.addItem(&holderWidget);
    holderWidget.setLayout(flowLayout);
    holderWidget.setContentsMargins(0,0,0,0);
}

ThumbnailWidget* FolderGridView::createThumbnailWidget() {
    ThumbnailWidget *widget = new ThumbnailWidget();
    widget->setPadding(8);
    ThumbnailStyle style = (settings->folderViewMode() == FV_SIMPLE) ? THUMB_SIMPLE : THUMB_NORMAL;
    widget->setThumbStyle(style);
    widget->setThumbnailSize(this->mThumbnailSize); // TODO: constructor
    return widget;
}

void FolderGridView::addItemToLayout(ThumbnailWidget* widget, int pos) {
    scene.addItem(widget);
    flowLayout->insertItem(pos, widget);
}

void FolderGridView::removeItemFromLayout(int pos) {
    flowLayout->removeAt(pos);
}

void FolderGridView::removeAll() {
    flowLayout->clear();
    qDeleteAll(thumbnails);
    thumbnails.clear();
}

void FolderGridView::updateLayout() {
    shiftedCol = -1;
    flowLayout->invalidate();
    flowLayout->activate();
}

// block native tab-switching so we can use it in shortcuts
bool FolderGridView::focusNextPrevChild(bool) {
    return false;
}

void FolderGridView::keyPressEvent(QKeyEvent *event) {
    ThumbnailView::keyPressEvent(event);
    event->accept();

    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit itemActivated(lastSelected());
        return;
    }

    // temporary, will be configurable later
    if(event->key() == Qt::Key_Backspace) {
        actionManager->invokeAction("goUp");
        return;
    }

    if(event->modifiers() & Qt::ControlModifier) {
        if(ShortcutBuilder::fromEvent(event) == "Ctrl+A")
            selectAll();
        else
            actionManager->processEvent(event);
        return;
    }

    // handle selection
    switch (event->key()) {
    case Qt::Key_Left:
        selectPrev();
        break;
    case Qt::Key_Right:
        selectNext();
        break;
    case Qt::Key_Up:
        selectAbove();
        break;
    case Qt::Key_Down:
        selectBelow();
        break;
    case Qt::Key_PageUp:
        pageUp();
        break;
    case Qt::Key_PageDown:
        pageDown();
        break;
    case Qt::Key_Home:
        selectFirst();
        break;
    case Qt::Key_End:
        selectLast();
        break;
    default:
        actionManager->processEvent(event);
    }
}

void FolderGridView::wheelEvent(QWheelEvent *event) {
    if(event->modifiers().testFlag(Qt::ControlModifier)) {
        if(event->pixelDelta().y() > 0 || event->angleDelta().y() > 0)
            zoomIn();
        else if(event->pixelDelta().y() < 0 || event->angleDelta().y() < 0)
            zoomOut();
    } else {
        ThumbnailView::wheelEvent(event);
    }
}

void FolderGridView::zoomIn() {
    setThumbnailSize(this->mThumbnailSize + ZOOM_STEP);
}

void FolderGridView::zoomOut() {
    setThumbnailSize(this->mThumbnailSize - ZOOM_STEP);
}

void FolderGridView::setThumbnailSize(int newSize) {
    newSize = clamp(newSize, THUMBNAIL_SIZE_MIN, THUMBNAIL_SIZE_MAX);
    if (newSize == mThumbnailSize) {
        return; // 避免重复设置，提高性能
    }
    
    mThumbnailSize = newSize;
    
    // 使用信号阻塞减少不必要的更新，提高批量操作性能
    blockSignals(true);
    for(int i = 0; i < thumbnails.count(); i++) {
        thumbnails.at(i)->setThumbnailSize(newSize);
    }
    blockSignals(false);
    
    updateLayout();
    fitSceneToContents();
    if(lastSelected() != -1)
        ensureVisible(thumbnails.at(lastSelected()), 0, 40);
    emit thumbnailSizeChanged(mThumbnailSize);
    loadVisibleThumbnails();
}

void FolderGridView::fitSceneToContents() {
    if(scrollBar->isVisible())
        holderWidget.setGeometry(0,0, width() - scrollBar->width(), height());
    else
        holderWidget.setGeometry(0,0, width(), height());
    ThumbnailView::fitSceneToContents();
}

void FolderGridView::resizeEvent(QResizeEvent *event) {
    if(this->isVisible()) {
        ThumbnailView::resizeEvent(event);
        fitSceneToContents();
        //focusOn(selectedIndex());
        loadVisibleThumbnailsDelayed();
    }
}

void FolderGridView::updateSelectionVisuals() {
    // 更新缩略图的选中状态显示
    for(int i = 0; i < thumbnails.count(); i++) {
        thumbnails.at(i)->setHighlighted(selectionBits.testBit(i));
    }
    updateScrollbarIndicator();
}

QList<int> FolderGridView::selection() {
    // 将QBitArray转换为QList<int>，保持与原来相同的行为
    QList<int> result;
    for (int i = 0; i < selectionBits.size(); ++i) {
        if (selectionBits.testBit(i)) {
            result.append(i);
        }
    }
    return result;
}

int FolderGridView::lastSelected() {
    // 返回最后选择的索引，与selection().last()行为一致
    if (selectionBits.count(true) == 0) {
        return -1;
    }
    // 找到最后设置的位（与selection().last()行为一致）
    for (int i = static_cast<int>(selectionBits.size()) - 1; i >= 0; --i) {
        if (selectionBits.testBit(i)) {
            return i;
        }
    }
    return -1;
}

void FolderGridView::select(QList<int> indices) {
    // 使用QBitArray高效实现选择
    selectionBits.fill(false, thumbnails.count());
    for (int index : indices) {
        if (checkRange(index)) {
            selectionBits.setBit(index);
        }
    }
    lastSelectedIndex = indices.isEmpty() ? -1 : indices.last();
    updateSelectionVisuals();
}

void FolderGridView::select(int index) {
    // 单个选择的优化实现
    if (!checkRange(index)) {
        return;
    }
    selectionBits.fill(false, thumbnails.count());
    selectionBits.setBit(index);
    lastSelectedIndex = index;
    updateSelectionVisuals();
}

void FolderGridView::clearSelection() {
    // 清除选择的高效实现
    selectionBits.fill(false);
    lastSelectedIndex = -1;
    updateSelectionVisuals();
}

void FolderGridView::deselect(int index) {
    // 取消选择的高效实现
    if (checkRange(index)) {
        selectionBits.setBit(index, false);
        updateSelectionVisuals();
    }
}