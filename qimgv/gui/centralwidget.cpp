#include "centralwidget.h"
#include <QVBoxLayout>

CentralWidget::CentralWidget(std::shared_ptr<DocumentWidget> docWidget,
                             QWidget *parent)
    : QWidget(parent),
      documentView(std::move(docWidget))
{
    setMouseTracking(true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(documentView.get());
}