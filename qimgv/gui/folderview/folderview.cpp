#include "folderview.h"
#include "ui_folderview.h"

FolderView::FolderView(QWidget *parent) :
    FloatingWidgetContainer(parent),
    ui(new Ui::FolderView)
{
    ui->setupUi(this);
    // 空实现
}

void FolderView::readSettings() {
    // 空实现
}

void FolderView::onSplitterMoved() {
    // 空实现
}

void FolderView::onPlacesPanelButtonChecked(bool mode) {
    Q_UNUSED(mode)
}

void FolderView::setPlacesPanel(bool mode) {
    Q_UNUSED(mode)
}

void FolderView::toggleBookmarks() {
    // 空实现
}

void FolderView::toggleFilesystemView() {
    // 空实现
}

void FolderView::onTreeViewTabOut() {
    // 空实现
}

// TODO: ask what to do
void FolderView::onDroppedInByIndex(QList<QString> paths, QModelIndex index) {
    Q_UNUSED(paths)
    Q_UNUSED(index)
}

void FolderView::onOptionsPopupButtonToggled(bool mode) {
    Q_UNUSED(mode)
}

void FolderView::onOptionsPopupDismissed() {
    // 空实现
}

void FolderView::onViewModeSelected(FolderViewMode mode) {
    Q_UNUSED(mode)
}

void FolderView::onThumbnailSizeChanged(int newSize) {
    Q_UNUSED(newSize)
}

void FolderView::onZoomSliderValueChanged(int value) {
    Q_UNUSED(value)
}

// changed by user via combobox
void FolderView::onSortingSelected(int mode) {
    Q_UNUSED(mode)
}

void FolderView::onSortingChanged(SortingMode mode) {
    Q_UNUSED(mode)
}

FolderView::~FolderView() {
    delete ui;
}

// probably unneeded
void FolderView::show() {
    QWidget::show();
}

// probably unneeded
void FolderView::hide() {
    QWidget::hide();
}

void FolderView::onFullscreenModeChanged(bool mode) {
    Q_UNUSED(mode)
}

void FolderView::focusInEvent(QFocusEvent *event) {
    Q_UNUSED(event)
}

void FolderView::populate(int count) {
    Q_UNUSED(count)
}

void FolderView::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    Q_UNUSED(pos)
    Q_UNUSED(thumb)
}

void FolderView::select(QList<int> indices) {
    Q_UNUSED(indices)
}

void FolderView::select(int index) {
    Q_UNUSED(index)
}

QList<int> FolderView::selection() {
    return QList<int>();
}

void FolderView::focusOn(int index) {
    Q_UNUSED(index)
}

void FolderView::focusOnSelection() {
    // 空实现
}

void FolderView::onHomeBtn() {
    // 空实现
}

void FolderView::onRootBtn() {
    // 空实现
}

void FolderView::setDirectoryPath(QString path) {
    Q_UNUSED(path)
}

void FolderView::fsTreeScrollToCurrent() {
    // 空实现
}

void FolderView::onTreeViewClicked(QModelIndex index) {
    Q_UNUSED(index)
}

void FolderView::onBookmarkClicked(QString dirPath) {
    Q_UNUSED(dirPath)
}

void FolderView::newBookmark() {
    // 空实现
}

void FolderView::addItem() {
    // 空实现
}

void FolderView::insertItem(int index) {
    Q_UNUSED(index)
}

void FolderView::removeItem(int index) {
    Q_UNUSED(index)
}

void FolderView::reloadItem(int index) {
    Q_UNUSED(index)
}

void FolderView::setDragHover(int index) {
    Q_UNUSED(index)
}

// prevent passthrough to parent
void FolderView::wheelEvent(QWheelEvent *event) {
    event->accept();
}

void FolderView::paintEvent(QPaintEvent *) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FolderView::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
}