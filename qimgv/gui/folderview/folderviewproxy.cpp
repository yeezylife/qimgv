#include "folderviewproxy.h"
#include <QApplication>

FolderViewProxy::FolderViewProxy(QWidget *parent)
    : QWidget(parent),
      folderView(nullptr)
{
    // 空实现
}

void FolderViewProxy::init() {
    // 空实现
}

void FolderViewProxy::populate(int count) {
    Q_UNUSED(count)
}

void FolderViewProxy::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    Q_UNUSED(pos)
    Q_UNUSED(thumb)
}

void FolderViewProxy::select(QList<int> indices) {
    Q_UNUSED(indices)
}

void FolderViewProxy::select(int index) {
    Q_UNUSED(index)
}

QList<int> FolderViewProxy::selection() {
    return QList<int>();
}

void FolderViewProxy::focusOn(int index) {
    Q_UNUSED(index)
}

void FolderViewProxy::focusOnSelection() {
    // 空实现
}

void FolderViewProxy::setDirectoryPath(QString path) {
    Q_UNUSED(path)
}

void FolderViewProxy::insertItem(int index) {
    Q_UNUSED(index)
}

void FolderViewProxy::removeItem(int index) {
    Q_UNUSED(index)
}

void FolderViewProxy::reloadItem(int index) {
    Q_UNUSED(index)
}

void FolderViewProxy::setDragHover(int index) {
    Q_UNUSED(index)
}

void FolderViewProxy::addItem() {
    // 空实现
}

void FolderViewProxy::onFullscreenModeChanged(bool mode) {
    Q_UNUSED(mode)
}

void FolderViewProxy::onSortingChanged(SortingMode mode) {
    Q_UNUSED(mode)
}

void FolderViewProxy::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
}