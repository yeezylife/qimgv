#include "thumbnailview.h"
#include <QApplication>
#include <QGuiApplication>

ThumbnailView::ThumbnailView(Qt::Orientation _orientation, QWidget *parent)
    : QGraphicsView(parent)
{
    Q_UNUSED(_orientation)
}

ThumbnailView::~ThumbnailView() {
}

Qt::Orientation ThumbnailView::orientation() {
    return Qt::Horizontal;
}

void ThumbnailView::setOrientation(Qt::Orientation _orientation) {
    Q_UNUSED(_orientation)
}

void ThumbnailView::hideEvent(QHideEvent *event) {
    QGraphicsView::hideEvent(event);
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
    Q_UNUSED(indices)
}

void ThumbnailView::select(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::deselect(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::addSelectionRange(int indexTo) {
    Q_UNUSED(indexTo)
}

QList<int> ThumbnailView::selection() {
    return QList<int>();
}

void ThumbnailView::clearSelection() {
}

int ThumbnailView::lastSelected() {
    return -1;
}

int ThumbnailView::itemCount() {
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
}

void ThumbnailView::addItem() {
}

void ThumbnailView::insertItem(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::removeItem(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::reloadItem(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::setDragHover(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::setCropThumbnails(bool mode) {
    Q_UNUSED(mode)
}

void ThumbnailView::setDrawScrollbarIndicator(bool mode) {
    Q_UNUSED(mode)
}

void ThumbnailView::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    Q_UNUSED(pos)
    Q_UNUSED(thumb)
}

void ThumbnailView::unloadAllThumbnails() {
}

void ThumbnailView::loadVisibleThumbnails() {
}

void ThumbnailView::loadVisibleThumbnailsDelayed() {
}

void ThumbnailView::resetViewport() {
}

int ThumbnailView::thumbnailSize() {
    return 0;
}

bool ThumbnailView::atSceneStart() {
    return true;
}

bool ThumbnailView::atSceneEnd() {
    return true;
}

bool ThumbnailView::checkRange(int pos) {
    Q_UNUSED(pos)
    return false;
}

void ThumbnailView::updateLayout() {
}

void ThumbnailView::fitSceneToContents() {
}

void ThumbnailView::wheelEvent(QWheelEvent *event) {
    QGraphicsView::wheelEvent(event);
}

void ThumbnailView::scrollPrecise(int delta) {
    Q_UNUSED(delta)
}

void ThumbnailView::scrollByItem(int delta) {
    Q_UNUSED(delta)
}

void ThumbnailView::scrollToItem(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration, bool additive) {
    Q_UNUSED(delta)
    Q_UNUSED(multiplier)
    Q_UNUSED(acceleration)
    Q_UNUSED(additive)
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration) {
    Q_UNUSED(delta)
    Q_UNUSED(multiplier)
    Q_UNUSED(acceleration)
}

void ThumbnailView::scrollSmooth(int delta) {
    Q_UNUSED(delta)
}

void ThumbnailView::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);
}

void ThumbnailView::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
}

void ThumbnailView::mouseReleaseEvent(QMouseEvent *event) {
    QGraphicsView::mouseReleaseEvent(event);
}

void ThumbnailView::mouseDoubleClickEvent(QMouseEvent *event) {
    QGraphicsView::mouseDoubleClickEvent(event);
}

void ThumbnailView::focusOutEvent(QFocusEvent *event) {
    QGraphicsView::focusOutEvent(event);
}

void ThumbnailView::keyPressEvent(QKeyEvent *event) {
    QGraphicsView::keyPressEvent(event);
}

void ThumbnailView::keyReleaseEvent(QKeyEvent *event) {
    QGraphicsView::keyReleaseEvent(event);
}

void ThumbnailView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
}

void ThumbnailView::setSelectMode(ThumbnailSelectMode mode) {
    Q_UNUSED(mode)
}
