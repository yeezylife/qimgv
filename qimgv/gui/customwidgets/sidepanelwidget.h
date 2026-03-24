#pragma once

#include <QWidget>

class SidePanelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SidePanelWidget(QWidget *parent = nullptr);

signals:

public slots:
    // 移除 = 0，使其不再是纯虚函数，避免链接错误
    // 保留 virtual 允许派生类重写
    virtual void show();
};