#pragma once

#include "gui/customwidgets/iconbutton.h"
#include "components/actionmanager/actionmanager.h"

enum TriggerMode {
    PressTrigger,
    ClickTrigger
};

class ActionButton : public IconButton {
public:
    explicit ActionButton(QWidget *parent = nullptr);

    // 万能兼容接口：去掉 &&，改为普通 QString
    // 这样既能接收 const QString& (老代码)，也能接收 QString&& (新代码)
    ActionButton(QString _actionName, QString _iconPath, QWidget *parent = nullptr);
    ActionButton(QString _actionName, QString _iconPath, int _size, QWidget *parent = nullptr);

    void setAction(QString _actionName);
    void setTriggerMode(TriggerMode mode);
    TriggerMode triggerMode();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    QString actionName;
    TriggerMode mTriggerMode = ClickTrigger;
};