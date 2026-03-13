#include "actionbutton.h"
#include <QMouseEvent>

ActionButton::ActionButton(QWidget *parent)
    : IconButton(parent)
    , actionName()
    , mTriggerMode(ClickTrigger)
{
    setFocusPolicy(Qt::NoFocus);
    setProperty("checked", false);
}

// 接收 QString 值副本。如果是左值则增加一次引用计数，如果是右值则直接移动。
ActionButton::ActionButton(QString _actionName, QString _iconPath, QWidget *parent)
    : ActionButton(parent)
{
    // 使用 std::move 将局部变量的所有权转交给成员/底层函数
    setAction(std::move(_actionName));
    setIconPath(_iconPath);
}

ActionButton::ActionButton(QString _actionName, QString _iconPath, int _size, QWidget *parent)
    : ActionButton(std::move(_actionName), std::move(_iconPath), parent)
{
    if(_size > 0)
        setFixedSize(_size, _size);
}

void ActionButton::setAction(QString _actionName) {
    // 将传入的副本直接移入成员变量，避免了额外的内存分配
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