#include "bookmarksitem.h"

BookmarksItem::BookmarksItem(QString _dirName, QString _dirPath, QWidget *parent) // NOLINT(bugprone-easily-swappable-parameters)
    : QWidget(parent), dirName(std::move(_dirName)), dirPath(std::move(_dirPath)), mHighlighted(false)
{
    // 空实现
}

QString BookmarksItem::path() {
    return dirPath;
}

void BookmarksItem::setHighlighted(bool mode) {
    Q_UNUSED(mode)
}

void BookmarksItem::mouseReleaseEvent(QMouseEvent *event) {
    event->accept();
}

void BookmarksItem::mousePressEvent(QMouseEvent *event) {
    event->accept();
}

void BookmarksItem::onRemoveClicked() {
    // 空实现
}

void BookmarksItem::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void BookmarksItem::dropEvent(QDropEvent *event) {
    Q_UNUSED(event)
}

void BookmarksItem::dragEnterEvent(QDragEnterEvent *event) {
    Q_UNUSED(event)
}

void BookmarksItem::dragLeaveEvent(QDragLeaveEvent *event) {
    Q_UNUSED(event)
}