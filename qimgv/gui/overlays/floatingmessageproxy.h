#pragma once

#include "gui/overlays/floatingmessage.h"

class FloatingMessageProxy
{
public:
    FloatingMessageProxy(FloatingWidgetContainer *parent);
    ~FloatingMessageProxy();
    void showMessage(const QString &text, FloatingMessageIcon icon, int duration);
    void showMessage(const QString &text, FloatingWidgetPosition position, FloatingMessageIcon icon, int duration);
    void init();
private:
    FloatingWidgetContainer *container;
    FloatingMessage *overlay;
};
