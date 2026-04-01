#include "foldergridview.h"

// TODO: create a base class for this and the one on panel

FolderGridView::FolderGridView(QWidget *parent)
    : ThumbnailView(Qt::Vertical, parent),
      shiftedCol(-1)
{
    // 空实现
}

void FolderGridView::dropEvent(QDropEvent *event) {
    Q_UNUSED(event)
}

void FolderGridView::dragEnterEvent(QDragEnterEvent *event) {
    Q_UNUSED(event)
}

void FolderGridView::dragMoveEvent(QDragMoveEvent *event) {
    Q_UNUSED(event)
}

void FolderGridView::dragLeaveEvent(QDragLeaveEvent *event) {
    Q_UNUSED(event)
}

void FolderGridView::setDragHover(int index) {
    Q_UNUSED(index)
}

void FolderGridView::onitemSelected() {
    // 空实现
}

void FolderGridView::updateScrollbarIndicator() {
    // 空实现
}

// probably unneeded
void FolderGridView::show() {
    QWidget::show();
}

// probably unneeded
void FolderGridView::hide() {
    QWidget::hide();
}

void FolderGridView::setShowLabels(bool mode) {
    Q_UNUSED(mode)
}

void FolderGridView::focusOnSelection() {
    // 空实现
}

void FolderGridView::selectAll() {
    // 空实现
}

void FolderGridView::selectAbove() {
    // 空实现
}

void FolderGridView::selectBelow() {
    // 空实现
}

void FolderGridView::selectNext() {
    // 空实现
}

void FolderGridView::selectPrev() {
    // 空实现
}

void FolderGridView::pageUp() {
    // 空实现
}

void FolderGridView::pageDown() {
    // 空实现
}

void FolderGridView::selectFirst() {
    // 空实现
}

void FolderGridView::selectLast() {
    // 空实现
}

void FolderGridView::scrollToCurrent() {
    // 空实现
}

// same as scrollToItem minus the animation
void FolderGridView::focusOn(int index) {
    Q_UNUSED(index)
}

void FolderGridView::setupLayout() {
    // 空实现
}

ThumbnailWidget* FolderGridView::createThumbnailWidget() {
    return nullptr;
}

void FolderGridView::addItemToLayout(ThumbnailWidget* widget, int pos) {
    Q_UNUSED(widget)
    Q_UNUSED(pos)
}

void FolderGridView::removeItemFromLayout(int pos) {
    Q_UNUSED(pos)
}

void FolderGridView::removeAll() {
    // 空实现
}

void FolderGridView::updateLayout() {
    // 空实现
}

// block native tab-switching so we can use it in shortcuts
bool FolderGridView::focusNextPrevChild(bool) {
    return false;
}

void FolderGridView::keyPressEvent(QKeyEvent *event) {
    Q_UNUSED(event)
}

void FolderGridView::wheelEvent(QWheelEvent *event) {
    Q_UNUSED(event)
}

void FolderGridView::zoomIn() {
    // 空实现
}

void FolderGridView::zoomOut() {
    // 空实现
}

void FolderGridView::setThumbnailSize(int newSize) {
    Q_UNUSED(newSize)
}

void FolderGridView::fitSceneToContents() {
    // 空实现
}

void FolderGridView::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
}

void FolderGridView::updateSelectionVisuals() {
    // 空实现
}

QList<int> FolderGridView::selection() {
    return QList<int>();
}

int FolderGridView::lastSelected() {
    return -1;
}

void FolderGridView::select(QList<int> indices) {
    Q_UNUSED(indices)
}

void FolderGridView::select(int index) {
    Q_UNUSED(index)
}

void FolderGridView::clearSelection() {
    // 空实现
}

void FolderGridView::deselect(int index) {
    Q_UNUSED(index)
}