#include "thumbnailview.h"
#include <QApplication>
#include <QGuiApplication>

ThumbnailView::ThumbnailView(Qt::Orientation _orientation, QWidget *parent)
    : QGraphicsView(parent)
{
    setAccessibleName("thumbnailView");
    this->setMouseTracking(true);
    this->setAcceptDrops(false);
    this->setScene(&scene);
    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    setAttribute(Qt::WA_TranslucentBackground, false);
    this->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    this->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);

    setOrientation(_orientation);

    loadTimer.stop();
    loadTimer.setInterval(static_cast<int>(LOAD_DELAY));
    loadTimer.setSingleShot(true);

    scrollRefreshRate = 1000; // minimal updates, no smooth scroll

    horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
}

Qt::Orientation ThumbnailView::orientation() {
    return mOrientation;
}

void ThumbnailView::setOrientation(Qt::Orientation _orientation) {
    mOrientation = _orientation;
    if(mOrientation == Qt::Horizontal) {
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollBar = this->horizontalScrollBar();
    } else {
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollBar = this->verticalScrollBar();
    }
}

void ThumbnailView::hideEvent(QHideEvent *event) {
    QGraphicsView::hideEvent(event);
    rangeSelection = false;
}

bool ThumbnailView::eventFilter(QObject *o, QEvent *ev) {
    Q_UNUSED(o)
    Q_UNUSED(ev)
    return false;
}

void ThumbnailView::setDirectoryPath(QString path) {
    Q_UNUSED(path)
}

void ThumbnailView::select(QList<int> indices) {
    mSelection = indices;
}

void ThumbnailView::select(int index) {
    if(!checkRange(index))
        index = 0;
    this->select(QList<int>() << index);
}

void ThumbnailView::deselect(int index) {
    mSelection.removeAll(index);
}

void ThumbnailView::addSelectionRange(int indexTo) {
    Q_UNUSED(indexTo)
    // 全禁用缩略图选择范围延展
}

QList<int> ThumbnailView::selection() {
    return mSelection;
}

void ThumbnailView::clearSelection() {
    mSelection.clear();
}

int ThumbnailView::lastSelected() {
    return mSelection.isEmpty() ? -1 : mSelection.last();
}

int ThumbnailView::itemCount() {
    Q_UNUSED(thumbnails)
    return 0;
}

void ThumbnailView::show() {
    QGraphicsView::show();
}

void ThumbnailView::showEvent(QShowEvent *event) {
    QGraphicsView::showEvent(event);
}

void ThumbnailView::populate(int newCount) {
    Q_UNUSED(newCount)
    clearSelection();
}

void ThumbnailView::addItem() {
    // disabled
}

void ThumbnailView::insertItem(int index) {
    Q_UNUSED(index)
    // disabled
}

void ThumbnailView::removeItem(int index) {
    Q_UNUSED(index)
    // disabled
}

void ThumbnailView::reloadItem(int index) {
    Q_UNUSED(index)
    // 缩略图功能已禁用，不再发出重载信号
}

void ThumbnailView::setDragHover(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::setCropThumbnails(bool mode) {
    mCropThumbnails = mode;
}

void ThumbnailView::setDrawScrollbarIndicator(bool mode) {
    mDrawScrollbarIndicator = mode;
}

void ThumbnailView::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    Q_UNUSED(pos)
    Q_UNUSED(thumb)
    // 缩略图功能已禁用
}

void ThumbnailView::unloadAllThumbnails() {
    // 全禁用缩略图，不进行卸载操作
}

void ThumbnailView::loadVisibleThumbnails() {
    // 彻底切断所有缩略图计算和请求
    loadTimer.stop();
}

void ThumbnailView::loadVisibleThumbnailsDelayed() {
    loadTimer.stop();
}

void ThumbnailView::resetViewport() {
    if(scrollBar)
        scrollBar->setValue(0);
}

int ThumbnailView::thumbnailSize() {
    return mThumbnailSize;
}

bool ThumbnailView::atSceneStart() {
    if(mOrientation == Qt::Horizontal) {
        return (viewportTransform().dx() == 0.0);
    }
    return (viewportTransform().dy() == 0.0);
}

bool ThumbnailView::atSceneEnd() {
    if(mOrientation == Qt::Horizontal) {
        return (viewportTransform().dx() <= viewport()->width() - sceneRect().width());
    }
    return (viewportTransform().dy() <= viewport()->height() - sceneRect().height());
}

bool ThumbnailView::checkRange(int pos) {
    return pos >= 0 && pos < static_cast<int>(thumbnails.count());
}

void ThumbnailView::updateLayout() {}

void ThumbnailView::fitSceneToContents() {
    // 缩略图已禁用，跳过布局计算
}

void ThumbnailView::wheelEvent(QWheelEvent *event) {
    Q_UNUSED(event)
    // 缩略图视图滚动已禁用
}

void ThumbnailView::scrollPrecise(int delta) {
    Q_UNUSED(delta)
    // 全禁用滚动
}

void ThumbnailView::scrollByItem(int delta) {
    Q_UNUSED(delta)
    // 全禁用滚动
}

void ThumbnailView::scrollToItem(int index) {
    Q_UNUSED(index)
    // 全禁用滚动
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration, bool additive) {
    Q_UNUSED(delta)
    Q_UNUSED(multiplier)
    Q_UNUSED(acceleration)
    Q_UNUSED(additive)
    // 全禁用滚动
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration) {
    scrollSmooth(delta, multiplier, acceleration, false);
}

void ThumbnailView::scrollSmooth(int delta) {
    scrollSmooth(delta, 1.0, 1.0, false);
}

void ThumbnailView::mousePressEvent(QMouseEvent *event) {
    mouseReleaseSelect = false;
    dragStartPos = QPoint(0,0);
    ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
    if(item) {
        int index = static_cast<int>(thumbnails.indexOf(item));
        if(event->button() == Qt::LeftButton) {
            if(event->modifiers() & Qt::ControlModifier) {
                if(!selection().contains(index)) select(selection() << index);
                else deselect(index);
            } else if(event->modifiers() & Qt::ShiftModifier) {
                addSelectionRange(index);
            } else if (selection().count() <= 1) {
                if(selectMode == ACTIVATE_BY_PRESS) {
                    emit itemActivated(index);
                    return;
                }
                select(index);
            } else mouseReleaseSelect = true;
            dragStartPos = event->position().toPoint();
        } else if(event->button() == Qt::RightButton) {
            select(index);
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void ThumbnailView::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    if(event->buttons() != Qt::LeftButton || mSelection.isEmpty()) return;
    if(QLineF(dragStartPos, event->position()).length() >= 40) {
        auto *item = dynamic_cast<ThumbnailWidget*>(itemAt(dragStartPos));
        if(item && mSelection.contains(static_cast<int>(thumbnails.indexOf(item))))
            emit draggedOut();
    }
}

void ThumbnailView::mouseReleaseEvent(QMouseEvent *event) {
    QGraphicsView::mouseReleaseEvent(event);
    if(mouseReleaseSelect && QLineF(dragStartPos, event->position()).length() < 40) {
        ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
        if(item) select(static_cast<int>(thumbnails.indexOf(item)));
    }
}

void ThumbnailView::mouseDoubleClickEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
        if(item) {
            emit itemActivated(static_cast<int>(thumbnails.indexOf(item)));
            return;
        }
    }
    event->ignore();
}

void ThumbnailView::focusOutEvent(QFocusEvent *event) {
    QGraphicsView::focusOutEvent(event);
    rangeSelection = false;
}

void ThumbnailView::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Shift) rangeSelectionSnapshot = selection();
    if(event->modifiers() & Qt::ShiftModifier) rangeSelection = true;
    QGraphicsView::keyPressEvent(event);
}

void ThumbnailView::keyReleaseEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Shift) rangeSelection = false;
    QGraphicsView::keyReleaseEvent(event);
}

void ThumbnailView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    updateScrollbarIndicator();
}

void ThumbnailView::setSelectMode(ThumbnailSelectMode mode) {
    selectMode = mode;
}