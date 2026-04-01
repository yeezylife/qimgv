#include "bookmarkswidget.h"

BookmarksWidget::BookmarksWidget(QWidget *parent) : QWidget(parent), highlightedPath("") {
    // 空实现
}

BookmarksWidget::~BookmarksWidget() = default;

void BookmarksWidget::readSettings() {
    // 空实现
}

void BookmarksWidget::saveBookmarks() {
    // 空实现
}

void BookmarksWidget::addBookmark(const QString &dirPath) {
    Q_UNUSED(dirPath)
}

void BookmarksWidget::removeBookmark(const QString &dirPath) {
    Q_UNUSED(dirPath)
}

void BookmarksWidget::onPathChanged(const QString &path) {
    Q_UNUSED(path)
}

void BookmarksWidget::dropEvent(QDropEvent *event) {
    Q_UNUSED(event)
}

void BookmarksWidget::dragEnterEvent(QDragEnterEvent *event) {
    Q_UNUSED(event)
}