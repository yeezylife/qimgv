#include "actionbutton.h"

ActionButton::ActionButton(QWidget *parent)
    : IconButton(parent)
    , actionName()
    , mTriggerMode(ClickTrigger)
{
    setFocusPolicy(Qt::NoFocus);
    setProperty("checked", false);
}

ActionButton::ActionButton(const QString &_actionName, const QString &_iconPath, QWidget *parent)
    : ActionButton(parent)
{
    // 既然 setIconPath 和 setAction 的接口是 const &
    // 这里直接传入即可，Qt 的引用计数机制会高效处理
    setIconPath(_iconPath);
    setAction(_actionName);
}

ActionButton::ActionButton(QString&& _actionName, QString&& _iconPath, int _size, QWidget *parent)
    :  ActionButton(std::move(_actionName), std::move(_iconPath), parent)
{
    if(_size > 0)
        setFixedSize(_size, _size);
}

void ActionButton::setAction(QString&& _actionName) {
    actionName = std::move(_actionName);
}

void ActionButton::setTriggerMode(TriggerMode mode) {
    mTriggerMode = mode;
}

TriggerMode ActionButton::triggerMode() {
    return mTriggerMode;
}

void ActionButton::mousePressEvent(QMouseEvent *event) {
    IconButton::mousePressEvent(event);
    if(mTriggerMode == TriggerMode::PressTrigger && event->button() == Qt::LeftButton)
        actionManager->invokeAction(actionName);
}

void ActionButton::mouseReleaseEvent(QMouseEvent *event) {
    IconButton::mouseReleaseEvent(event);
    if(mTriggerMode == TriggerMode::ClickTrigger && rect().contains(event->position().toPoint()) && event->button() == Qt::LeftButton)
        actionManager->invokeAction(actionName);
}
