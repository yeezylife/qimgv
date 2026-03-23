#include "centralwidget.h"

CentralWidget::CentralWidget(std::shared_ptr<DocumentWidget> docWidget,
                             std::shared_ptr<FolderViewProxy> folderView,
                             QWidget *parent)
    : QStackedWidget(parent),
      documentView(std::move(docWidget)),
      folderView(std::move(folderView)),
      mode(MODE_DOCUMENT)
{
    setMouseTracking(true);
    addWidget(documentView.get());
    if (this->folderView) {
        addWidget(this->folderView.get());
    }

    showDocumentView();
}

void CentralWidget::showDocumentView() {
    switchTo(0, MODE_DOCUMENT);
}

void CentralWidget::showFolderView() {
    switchTo(1, MODE_FOLDERVIEW);
}

void CentralWidget::toggleViewMode() {
    if (mode == MODE_DOCUMENT)
        showFolderView();
    else
        showDocumentView();
}

ViewMode CentralWidget::currentViewMode() {
    return mode;
}

void CentralWidget::switchTo(int index, ViewMode newMode)
{
    bool modeChanged = (mode != newMode);
    bool indexChanged = (currentIndex() != index);

    // 如果模式没变但 index 变了，也应该更新 UI
    if (!modeChanged && !indexChanged)
        return;

    mode = newMode;
    setCurrentIndex(index);
    QWidget *w = widget(index);
    if (w) {
        w->show();
        w->setFocus();
    }

    if (newMode == MODE_DOCUMENT)
        documentView->viewWidget()->startPlayback();
    else
        documentView->viewWidget()->stopPlayback();
}