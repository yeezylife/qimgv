#include "contextmenuitem.h"

ContextMenuItem::ContextMenuItem(QWidget *parent)
    : MenuItem(parent),
      mAction("")
{
}

ContextMenuItem::~ContextMenuItem() = default;

void ContextMenuItem::setAction(QString text) {
    mAction = std::move(text);
    setShortcutText(actionManager->shortcutForAction(mAction));
}

void ContextMenuItem::onPress() {
    emit pressed();
    if(!mAction.isEmpty())
        actionManager->invokeAction(mAction);
}
