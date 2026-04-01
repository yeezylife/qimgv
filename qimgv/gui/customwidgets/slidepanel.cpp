#include "slidepanel.h"

SlidePanel::SlidePanel(FloatingWidgetContainer *parent)
    : FloatingWidget(parent),
      panelSize(50),
      slideAmount(40),
      mWidget(nullptr),
      mPosition(PANEL_TOP)
{
    mLayout.setSpacing(0);
    mLayout.setContentsMargins(0, 0, 0, 0);
    this->setLayout(&mLayout);
    fadeEffect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(fadeEffect);
    startPosition = geometry().topLeft();
    outCurve.setType(QEasingCurve::OutQuart);
    timeline.setDuration(ANIMATION_DURATION);
    timeline.setEasingCurve(QEasingCurve::Linear);
    timeline.setStartFrame(0);
    timeline.setEndFrame(ANIMATION_DURATION);
    this->setAttribute(Qt::WA_NoMousePropagation, true);
    this->setFocusPolicy(Qt::NoFocus);
    mLayout.setDirection(QBoxLayout::LeftToRight);
    QWidget::hide();
}

SlidePanel::~SlidePanel() = default;

void SlidePanel::hide() {
    QWidget::hide();
}

void SlidePanel::hideAnimated() {
    QWidget::hide();
}

bool SlidePanel::layoutManaged() {
    return mLayoutManaged;
}

void SlidePanel::setLayoutManaged(bool mode) {
    mLayoutManaged = mode;
}

void SlidePanel::setWidget(const std::shared_ptr<QWidget>& w) {
    mWidget = w;
}

bool SlidePanel::hasWidget() {
    return (mWidget != nullptr);
}

void SlidePanel::show() {
    QWidget::show();
}

void SlidePanel::saveStaticGeometry(QRect geometry) {
    mStaticGeometry = geometry;
}

QRect SlidePanel::staticGeometry() {
    return mStaticGeometry;
}

void SlidePanel::animationUpdate(int /*frame*/) {
}

void SlidePanel::setAnimationRange(QPoint start, QPoint end) {
    startPosition = start;
    endPosition = end;
}

void SlidePanel::onAnimationFinish() {
}

QRect SlidePanel::triggerRect() {
    return mTriggerRect;
}

void SlidePanel::setPosition(PanelPosition p) {
    mPosition = p;
}

PanelPosition SlidePanel::position() {
    return mPosition;
}

void SlidePanel::recalculateGeometry() {
}

void SlidePanel::recalculateGeometryInternal() {
}

void SlidePanel::updateTriggerRect() {
}

void SlidePanel::updateTriggerRectInternal() {
}

void SlidePanel::setOrientation() {
}