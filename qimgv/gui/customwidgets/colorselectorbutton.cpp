#include "colorselectorbutton.h"

ColorSelectorButton::ColorSelectorButton(QWidget *parent) : ClickableLabel(parent) {
    connect(this, &ColorSelectorButton::clicked, this, &ColorSelectorButton::showColorSelector);
}

void ColorSelectorButton::setColor(QColor &newColor) {
    mColor = newColor;
    update();
}

void ColorSelectorButton::setDescription(QString text) {
    mDescription = text;
}

QColor ColorSelectorButton::color() {
    return mColor;
}

void ColorSelectorButton::showColorSelector() {
    QColor newColor = QColorDialog::getColor(mColor, this, mDescription);
    if(newColor.isValid()) {
        mColor = newColor;
        update();
    }
}

void ColorSelectorButton::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if(!isEnabled())
        p.setOpacity(0.5f);
    p.setPen(QColor(40,40,40));
    p.drawRect(QRectF(0.5f, 0.5f, static_cast<float>(width()) - 1.0f, static_cast<float>(height()) - 1.0f));
    p.fillRect(rect().adjusted(2,2,-2,-2), mColor);
}
