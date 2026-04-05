#include "controlsoverlay.h"

ControlsOverlay::ControlsOverlay(FloatingWidgetContainer *parent) :
    FloatingWidget(parent)
{
    minimizeButton = new ActionButton("minimize", ":/res/icons/common/buttons/panel/minimize16.png", 30);
    minimizeButton->setAccessibleName("ButtonSmall");
    windowModeButton = new ActionButton("toggleFullscreen", ":/res/icons/common/buttons/panel/maximize16.png", 30);
    windowModeButton->setAccessibleName("ButtonSmall");
    closeButton = new ActionButton("exit", ":/res/icons/common/buttons/panel/close16.png", 30);
    closeButton->setAccessibleName("ButtonSmall");

    layout.setContentsMargins(0,0,0,0);
    this->setContentsMargins(0,0,0,0);
    layout.setSpacing(0);
    layout.addWidget(minimizeButton);
    layout.addWidget(windowModeButton);
    layout.addWidget(closeButton);
    setLayout(&layout);
    
    // 构造函数中直接设置大小和几何，避免调用虚函数
    mCachedContentsSize = QSize(0, 0);
    for(int i=0; i<layout.count(); i++) {
        mCachedContentsSize.setWidth(mCachedContentsSize.width() + layout.itemAt(i)->widget()->width());
        mCachedContentsSize.setHeight(layout.itemAt(i)->widget()->height());
    }
    this->setFixedSize(mCachedContentsSize);
    recalculateGeometryInternal();

    setMouseTracking(true);

    fadeEffect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(fadeEffect);
    fadeAnimation = new QPropertyAnimation(fadeEffect, "opacity");
    fadeAnimation->setDuration(230);
    fadeAnimation->setStartValue(1.0f);
    fadeAnimation->setEndValue(0);
    fadeAnimation->setEasingCurve(QEasingCurve::OutQuart);

    if(parent)
        setContainerSize(parent->size());
    //this->show();
}

void ControlsOverlay::show() {
    fadeEffect->setOpacity(0.0);
    FloatingWidget::show();
}

QSize ControlsOverlay::contentsSize() {
    return mCachedContentsSize;
}

void ControlsOverlay::fitToContents() {
    this->setFixedSize(mCachedContentsSize);
    recalculateGeometry();
}

void ControlsOverlay::recalculateGeometryInternal() {
    setGeometry(containerSize().width() - width(), 0, width(), height());
}

void ControlsOverlay::recalculateGeometry() {
    recalculateGeometryInternal();
}

#if QT_VERSION > QT_VERSION_CHECK(6,0,0)
void ControlsOverlay::enterEvent(QEnterEvent *event) {
#else
void ControlsOverlay::enterEvent(QEvent *event) {
#endif
    Q_UNUSED(event)
    fadeAnimation->stop();
    fadeEffect->setOpacity(1.0);
}

void ControlsOverlay::leaveEvent(QEvent *event) {
    Q_UNUSED(event)
    fadeAnimation->start();
}