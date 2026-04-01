#include "thumbnailstrip.h"

ThumbnailStrip::ThumbnailStrip(QWidget *parent) : ThumbnailView(Qt::Horizontal, parent) {
}

void ThumbnailStrip::updateScrollbarIndicator() {
}

void ThumbnailStrip::setupLayout() {
}

ThumbnailWidget* ThumbnailStrip::createThumbnailWidget() {
    return nullptr;
}

void ThumbnailStrip::addItemToLayout(ThumbnailWidget* widget, int pos) {
    Q_UNUSED(widget)
    Q_UNUSED(pos)
}

void ThumbnailStrip::removeItemFromLayout(int pos) {
    Q_UNUSED(pos)
}

void ThumbnailStrip::removeAll() {
}

void ThumbnailStrip::updateThumbnailPositions() {
}

void ThumbnailStrip::updateThumbnailPositions(int start, int end) {
    Q_UNUSED(start)
    Q_UNUSED(end)
}

void ThumbnailStrip::focusOn(int index) {
    Q_UNUSED(index)
}

void ThumbnailStrip::focusOnSelection() {
}

void ThumbnailStrip::readSettings() {
}

QSize ThumbnailStrip::itemSize() {
    return QSize();
}

void ThumbnailStrip::resizeEvent(QResizeEvent *event) {
    ThumbnailView::resizeEvent(event);
}
