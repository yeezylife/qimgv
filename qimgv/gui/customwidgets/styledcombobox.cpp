#include "styledcombobox.h"

StyledComboBox::StyledComboBox(QWidget *parent)
    : QComboBox(parent)
    , hiResPixmap(false)
    , dpr(devicePixelRatioF())
    , pixmapDrawScale(1.0)
{
    connect(settings, &Settings::settingsChanged, this, [this]() {
        ImageLib::recolor(this->downArrow, settings->colorScheme().icons);
    });
}

void StyledComboBox::setIconPath(const QString& path) {
    QString actualPath = path;
    if (dpr >= 1.001) {
        actualPath.replace(".", "@2x.");
        hiResPixmap = true;
        downArrow.load(actualPath);
        pixmapDrawScale = (dpr >= 1.999) ? dpr : 2.0;
        downArrow.setDevicePixelRatio(pixmapDrawScale);
    } else {
        hiResPixmap = false;
        downArrow.load(actualPath);
        pixmapDrawScale = dpr;
    }
    ImageLib::recolor(downArrow, settings->colorScheme().icons);
    update();
}

void StyledComboBox::paintEvent(QPaintEvent *e) {
    QComboBox::paintEvent(e);
    QPainter p(this);
    QPointF pos;

    if (hiResPixmap) {
        pos = QPointF(width() - 8 - downArrow.width() / pixmapDrawScale,
                      height() / 2 - downArrow.height() / (2 * pixmapDrawScale));
    } else {
        pos = QPointF(width() - downArrow.width() - 8,
                      (height() - downArrow.height()) / 2);
    }
    p.drawPixmap(pos, downArrow);
}

void StyledComboBox::keyPressEvent(QKeyEvent *event) {
    event->ignore();
}