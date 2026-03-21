#pragma once

#include "gui/overlays/imageinfooverlay.h"

struct ImageInfoOverlayStateBuffer {
    QHash<QString, QString> info;
};

class ImageInfoOverlayProxy {
public:
    explicit ImageInfoOverlayProxy(FloatingWidgetContainer *parent = nullptr);
    ~ImageInfoOverlayProxy();
    void init();
    void show();
    void hide();

    void setExifInfo(const QHash<QString, QString>& _info);
    bool isHidden();
private:
    FloatingWidgetContainer *container;
    ImageInfoOverlay *overlay;
    ImageInfoOverlayStateBuffer stateBuf;
};
