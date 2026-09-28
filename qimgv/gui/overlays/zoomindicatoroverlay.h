#pragma once

#include "gui/customwidgets/overlaywidget.h"
#include <QFont>
#include <QFontMetrics>
#include <QTimer>

class ZoomIndicatorOverlay : public OverlayWidget {
    Q_OBJECT
public:
    explicit ZoomIndicatorOverlay(FloatingWidgetContainer *parent = nullptr);

    using OverlayWidget::show;   // 恢复基类 show() 重载（show(int) 会隐藏基类版本）
    void setScale(qreal scale);
    void show(int duration);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCache();          // 更新缓存的文本尺寸和字体度量

    QTimer visibilityTimer;
    int hideDelay = 2000;

    QString m_text;
    QFontMetrics m_fm;
    QFont m_cachedFont;
    int m_lastScalePct = -1; // setScale 无分配早退用：缩放百分比非负，-1 为哨兵
    int m_textWidth = 0;
    int m_ascent = 0;
    int m_descent = 0;
};