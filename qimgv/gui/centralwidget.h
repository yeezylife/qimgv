#pragma once

#include <QWidget>
#include "gui/viewers/documentwidget.h"

class CentralWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CentralWidget(std::shared_ptr<DocumentWidget> docWidget,
                           QWidget *parent = nullptr);

    std::shared_ptr<DocumentWidget> documentViewWidget() const { return documentView; }

signals:

public slots:

private:
    std::shared_ptr<DocumentWidget> documentView;
};
