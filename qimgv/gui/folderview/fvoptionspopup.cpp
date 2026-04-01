#include "fvoptionspopup.h"
#include "ui_fvoptionspopup.h"

FVOptionsPopup::FVOptionsPopup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FVOptionsPopup)
{
    ui->setupUi(this);
    // 空实现
}

FVOptionsPopup::~FVOptionsPopup() {
    delete ui;
}

void FVOptionsPopup::setSimpleView() {
    // 空实现
}

void FVOptionsPopup::setExtendedView() {
    // 空实现
}

void FVOptionsPopup::setFoldersView() {
    // 空实现
}

void FVOptionsPopup::selectSimpleView() {
    // 空实现
}

void FVOptionsPopup::selectExtendedView() {
    // 空实现
}

void FVOptionsPopup::selectFoldersView() {
    // 空实现
}

void FVOptionsPopup::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FVOptionsPopup::keyPressEvent(QKeyEvent *event) {
    Q_UNUSED(event)
}

void FVOptionsPopup::setViewMode(FolderViewMode mode) {
    Q_UNUSED(mode)
}

void FVOptionsPopup::readSettings() {
    // 空实现
}

void FVOptionsPopup::showAt(QPoint pos) {
    Q_UNUSED(pos)
}

void FVOptionsPopup::hideEvent(QHideEvent* event) {
    event->accept();
}