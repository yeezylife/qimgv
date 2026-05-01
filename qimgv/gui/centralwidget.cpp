#include "centralwidget.h"
#include <QVBoxLayout>

CentralWidget::CentralWidget(DocumentWidget *docWidget,
                             QWidget *parent)
    : QWidget(parent),
      documentView(docWidget)
{
    setMouseTracking(true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(documentView);
}