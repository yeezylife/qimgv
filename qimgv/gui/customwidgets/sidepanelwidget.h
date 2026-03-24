#pragma once

#include <QWidget>

class SidePanelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SidePanelWidget(QWidget *parent = nullptr);
    virtual ~SidePanelWidget() = default;

public slots:
    virtual void show();
};