#include "treeviewcustom.h"

TreeViewCustom::TreeViewCustom(QWidget *parent) : QTreeView(parent) {
    // 空实现
}

void TreeViewCustom::dropEvent(QDropEvent *event) {
    Q_UNUSED(event)
}

void TreeViewCustom::dragEnterEvent(QDragEnterEvent *event) {
    Q_UNUSED(event)
}

void TreeViewCustom::showEvent(QShowEvent *event) {
    QTreeView::showEvent(event);
}

void TreeViewCustom::enterEvent(QEnterEvent *event) {
    QTreeView::enterEvent(event);
}

void TreeViewCustom::leaveEvent(QEvent *event) {
    QTreeView::leaveEvent(event);
}

QSize TreeViewCustom::minimumSizeHint() const {
    return QTreeView::minimumSizeHint();
}

void TreeViewCustom::resizeEvent(QResizeEvent *event) {
    QTreeView::resizeEvent(event);
}

void TreeViewCustom::updateScrollbarStyle() {
    // 空实现
}

void TreeViewCustom::keyPressEvent(QKeyEvent* event) {
    QTreeView::keyPressEvent(event);
}