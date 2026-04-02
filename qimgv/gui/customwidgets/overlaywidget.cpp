#include "overlaywidget.h"

OverlayWidget::OverlayWidget(FloatingWidgetContainer *parent)
    : FloatingWidget(parent),
      mHorizontalMargin(20),
      mVerticalMargin(35),
      fadeEnabled(false)
{
    opacityEffect = new QGraphicsOpacityEffect(this);
    opacityEffect->setOpacity(1.0);
    setGraphicsEffect(opacityEffect);
    fadeAnimation = new QPropertyAnimation(this, "opacity");
    fadeAnimation->setDuration(120);
    fadeAnimation->setStartValue(1.0f);
    fadeAnimation->setEndValue(0.0f);
    fadeAnimation->setEasingCurve(QEasingCurve::OutQuad);
    connect(fadeAnimation, &QPropertyAnimation::finished, [this]() {
        QWidget::hide();
    });
}

OverlayWidget::~OverlayWidget() {
    delete opacityEffect;
    delete fadeAnimation;
}

qreal OverlayWidget::opacity() const {
    return opacityEffect->opacity();
}

void OverlayWidget::setOpacity(qreal opacity) {
    opacityEffect->setOpacity(opacity);
}

void OverlayWidget::setHorizontalMargin(int margin) {
    mHorizontalMargin = margin;
    recalculateGeometry();
}

void OverlayWidget::setVerticalMargin(int margin) {
    mVerticalMargin = margin;
    recalculateGeometry();
}

int OverlayWidget::horizontalMargin() {
    return mHorizontalMargin;
}

int OverlayWidget::verticalMargin() {
    return mVerticalMargin;
}

void OverlayWidget::setPosition(FloatingWidgetPosition pos) {
    position = pos;
    recalculateGeometry();
}

void OverlayWidget::setFadeDuration(int duration) {
    fadeAnimation->setDuration(duration);
}

void OverlayWidget::setFadeEnabled(bool mode) {
    fadeEnabled = mode;
}

void OverlayWidget::show() {
    fadeAnimation->stop();
    opacityEffect->setOpacity(1.0);
    FloatingWidget::show();
}

void OverlayWidget::hideAnimated() {
    if(fadeEnabled && !this->isHidden()) {
        fadeAnimation->stop();
        fadeAnimation->start(QPropertyAnimation::KeepWhenStopped);
    } else {
        FloatingWidget::hide();
    }
}

void OverlayWidget::hide() {
    fadeAnimation->stop();
    FloatingWidget::hide();
}

void OverlayWidget::recalculateGeometry() {
    const QSize containerSz = containerSize();
    QRect newRect = QRect(QPoint(0,0), sizeHint());
    QPoint pos(0, 0);
    switch (position) {
        case FloatingWidgetPosition::LEFT:
            pos.setX(mHorizontalMargin);
            pos.setY((containerSz.height() - newRect.height()) / 2);
            break;
        case FloatingWidgetPosition::RIGHT:
            pos.setX(containerSz.width() - newRect.width() - mHorizontalMargin);
            pos.setY((containerSz.height() - newRect.height()) / 2);
            break;
        case FloatingWidgetPosition::BOTTOM:
            pos.setX((containerSz.width() - newRect.width()) / 2);
            pos.setY(containerSz.height() - newRect.height() - mVerticalMargin);
            break;
        case FloatingWidgetPosition::TOP:
            pos.setX((containerSz.width() - newRect.width()) / 2);
            pos.setY(mVerticalMargin);
            break;
        case FloatingWidgetPosition::TOPLEFT:
            pos.setX(mHorizontalMargin);
            pos.setY(mVerticalMargin);
            break;
        case FloatingWidgetPosition::TOPRIGHT:
            pos.setX(containerSz.width() - newRect.width() - mHorizontalMargin);
            pos.setY(mVerticalMargin);
            break;
        case FloatingWidgetPosition::BOTTOMLEFT:
            pos.setX(mHorizontalMargin);
            pos.setY(containerSz.height() - newRect.height() - mVerticalMargin);
            break;
        case FloatingWidgetPosition::BOTTOMRIGHT:
            pos.setX(containerSz.width() - newRect.width() - mHorizontalMargin);
            pos.setY(containerSz.height() - newRect.height() - mVerticalMargin);
            break;
        case FloatingWidgetPosition::CENTER:
            pos.setX((containerSz.width() - newRect.width()) / 2);
            pos.setY((containerSz.height() - newRect.height()) / 2);
    }
    newRect.moveTopLeft(pos);
    setGeometry(newRect);
}
