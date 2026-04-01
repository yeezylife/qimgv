#include "thumbnailstripproxy.h"

ThumbnailStripProxy::ThumbnailStripProxy(QWidget *parent)
    : QWidget(parent)
{
}

ThumbnailStripProxy::~ThumbnailStripProxy() {
}

void ThumbnailStripProxy::init() {
}

bool ThumbnailStripProxy::isInitialized() {
    return false;
}

void ThumbnailStripProxy::populate(int count) {
    Q_UNUSED(count)
}

void ThumbnailStripProxy::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    Q_UNUSED(pos)
    Q_UNUSED(thumb)
}

void ThumbnailStripProxy::select(QList<int> indices) {
    Q_UNUSED(indices)
}

void ThumbnailStripProxy::select(int index) {
    Q_UNUSED(index)
}

QList<int> ThumbnailStripProxy::selection() {
    return QList<int>();
}

void ThumbnailStripProxy::focusOn(int index) {
    Q_UNUSED(index)
}

void ThumbnailStripProxy::focusOnSelection() {
}

void ThumbnailStripProxy::insertItem(int index) {
    Q_UNUSED(index)
}

void ThumbnailStripProxy::removeItem(int index) {
    Q_UNUSED(index)
}

void ThumbnailStripProxy::reloadItem(int index) {
    Q_UNUSED(index)
}

void ThumbnailStripProxy::setDragHover(int index) {
    Q_UNUSED(index)
}

void ThumbnailStripProxy::setDirectoryPath(QString path) {
    Q_UNUSED(path)
}

void ThumbnailStripProxy::addItem() {
}

QSize ThumbnailStripProxy::itemSize() {
    return QSize();
}

void ThumbnailStripProxy::readSettings() {
}

void ThumbnailStripProxy::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
}
