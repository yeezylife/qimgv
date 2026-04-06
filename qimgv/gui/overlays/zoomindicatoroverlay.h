#pragma once

#include "gui/customwidgets/overlaywidget.h"
#include <QTimer>

class ZoomIndicatorOverlay : public OverlayWidget {
    Q_OBJECT
public:
    explicit ZoomIndicatorOverlay(FloatingWidgetContainer *parent = nullptr);

    void setScale(qreal scale);
    void show();
    void show(int duration);

protected:
    void recalculateGeometry() override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCache();          // 更新缓存的文本尺寸和字体度量

    QTimer visibilityTimer;
    int hideDelay = 2000;

    QString m_text;
    QFontMetrics m_fm;
    int m_textWidth = 0;
    int m_ascent = 0;
    int m_descent = 0;
};