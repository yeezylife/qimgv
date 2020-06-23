#include "styledcombobox.h"

StyledComboBox::StyledComboBox(QWidget *parent) : QComboBox(parent), hiResPixmap(false)
{
    dpr = this->devicePixelRatioF();
}

void StyledComboBox::setIconPath(QString path) {
    if(dpr >= (1.0 + 0.001)) {
        path.replace(".", "@2x.");
        hiResPixmap = true;
        downArrow.load(path);
        if(dpr >= (2.0 - 0.001))
            pixmapDrawScale = dpr;
        else
            pixmapDrawScale = 2.0;
        downArrow.setDevicePixelRatio(pixmapDrawScale);
    } else {
        hiResPixmap = false;
        downArrow.load(path);
        pixmapDrawScale = dpr;
    }
    ImageLib::overlayColor(downArrow, settings->colorScheme().text);
    update();
}

void StyledComboBox::paintEvent(QPaintEvent *e) {
    QComboBox::paintEvent(e);
    QPainter p(this);
    QPointF pos(0,0);

    if(hiResPixmap) {
        pos = QPointF(width() - 8 - downArrow.width() / pixmapDrawScale,
                      height() / 2 - downArrow.height() / (2 * pixmapDrawScale));
    } else {
        pos = QPointF(width() - downArrow.width() - 8,
                      (height() - downArrow.height()) / 2);
    }
    p.drawPixmap(pos, downArrow);
}
