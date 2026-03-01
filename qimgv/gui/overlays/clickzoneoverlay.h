#pragma once

#include <QHBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDebug>
#include "gui/customwidgets/floatingwidget.h"
#include "utils/imagelib.h"

enum ActiveHighlightZone {
    HIGHLIGHT_NONE,
    HIGHLIGHT_LEFT,
    HIGHLIGHT_RIGHT
};

class ClickZoneOverlay : public FloatingWidget {
    Q_OBJECT
public:
    explicit ClickZoneOverlay(FloatingWidgetContainer *parent);
    QRect leftZone() const;
    QRect rightZone() const;
    void highlightLeft();
    void highlightRight();
    void disableHighlight();
    void setHighlightedZone(ActiveHighlightZone zone);
    bool isHighlighted() const;
    void setPressed(bool mode);

public slots:
    void readSettings();

private:
    QPixmap loadPixmap(const QString& path);
    QPixmap pixmapLeft;
    QPixmap pixmapRight;
    QRect mLeftZone, mRightZone;
    qreal dpr = 1.0;
    qreal pixmapDrawScale = 1.0;
    bool hiResPixmaps = false;
    const int zoneSize = 110;
    bool isPressed = false;
    bool drawZones = true;
    ActiveHighlightZone activeZone = HIGHLIGHT_NONE;

    void drawPixmap(QPainter &p, const QPixmap& pixmap, const QRect& rect);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void recalculateGeometry() override;
};