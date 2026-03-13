// Base class for various menu items.
// Displays entry name, shortcut and an icon.

#pragma once

#include <QLabel>
#include <QStyleOption>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QPainter>
#include "gui/customwidgets/iconbutton.h"
#include "components/actionmanager/actionmanager.h"

class MenuItem : public QWidget {
    Q_OBJECT
public:
    MenuItem(QWidget *parent = nullptr);
    ~MenuItem();

    // 修复：全部改为 const QString & 以消除 performance-unnecessary-value-param 警告
    void setText(const QString &text);
    QString text();

    void setShortcutText(const QString &text);
    QString shortcut();

    void setIconPath(const QString &path);
    void setPassthroughClicks(bool mode);

protected:
    IconButton mIconWidget;
    QLabel mTextLabel, mShortcutLabel;
    QSpacerItem *spacer;
    QHBoxLayout mLayout;
    bool passthroughClicks = true;

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    virtual void onClick();
    virtual void onPress();
};