#pragma once

#include <QLabel>
#include <QStyleOption>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QPainter>
#include <utility> // 必须包含此头文件以使用 std::move
#include "gui/customwidgets/iconbutton.h"
#include "components/actionmanager/actionmanager.h"

class MenuItem : public QWidget {
    Q_OBJECT
public:
    MenuItem(QWidget *parent = nullptr);
    ~MenuItem();

    // 修复：改为 const & (针对只读引用的警告)
    void setText(const QString &text);
    QString text();

    void setShortcutText(const QString &text);
    QString shortcut();

    // 修复：按编译器建议使用值传递 + move (针对 Sink 模式的警告)
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