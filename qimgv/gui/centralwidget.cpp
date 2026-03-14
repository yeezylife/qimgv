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
    // docWidget - 0, folderView - 1
    addWidget(documentView.get());
    if (folderView)
        addWidget(folderView.get());
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
    if (mode == newMode)
        return;
    mode = newMode;
    setCurrentIndex(index);
    QWidget *w = widget(index);
    if (w) {
        w->setFocus();
    }
    if (newMode == MODE_DOCUMENT)
        documentView->viewWidget()->startPlayback();
    else
        documentView->viewWidget()->stopPlayback();
}