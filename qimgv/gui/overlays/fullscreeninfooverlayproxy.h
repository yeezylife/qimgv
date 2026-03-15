#pragma once

#include "gui/overlays/fullscreeninfooverlay.h"

struct InfoOverlayStateBuffer {
    QString position;
    QString fileName;
    QString info;
    bool showImmediately = false;
};

class FullscreenInfoOverlayProxy {
public:
    explicit FullscreenInfoOverlayProxy(FloatingWidgetContainer *parent = nullptr);
    ~FullscreenInfoOverlayProxy();
    void init();
    void show();
    void showWhenReady();
    void hide();
    void setInfo(const QString &_position, const QString &_fileName, const QString &_info);

private:
    FloatingWidgetContainer *container;
    FullscreenInfoOverlay *infoOverlay;
    InfoOverlayStateBuffer stateBuf;
};
