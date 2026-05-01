#pragma once

#include <QWidget>
#include "gui/viewers/documentwidget.h"

class CentralWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CentralWidget(DocumentWidget *docWidget,
                           QWidget *parent = nullptr);

    DocumentWidget *documentViewWidget() const { return documentView; }

signals:

public slots:

private:
    DocumentWidget *documentView = nullptr;
};
