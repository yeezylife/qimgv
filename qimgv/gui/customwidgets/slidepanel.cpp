#include "slidepanel.h"

SlidePanel::SlidePanel(FloatingWidgetContainer *parent)
    : FloatingWidget(parent),
      panelSize(50),
      slideAmount(40),
      mWidget(nullptr),
      mPosition(PANEL_TOP) // 显式初始化成员
{
    mLayout.setSpacing(0);
    mLayout.setContentsMargins(0, 0, 0, 0);
    this->setLayout(&mLayout);

    // Fade effect
    fadeEffect = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(fadeEffect);

    startPosition = geometry().topLeft();
    outCurve.setType(QEasingCurve::OutQuart);

    timeline.setDuration(ANIMATION_DURATION);
    timeline.setEasingCurve(QEasingCurve::Linear);
    timeline.setStartFrame(0);
    timeline.setEndFrame(ANIMATION_DURATION);

#ifdef _WIN32
    timeline.setUpdateInterval(8);
#else
    timeline.setUpdateInterval(16);
#endif

    connect(&timeline, &QTimeLine::frameChanged, this, &SlidePanel::animationUpdate);
    connect(&timeline, &QTimeLine::finished, this, &SlidePanel::onAnimationFinish);

    this->setAttribute(Qt::WA_NoMousePropagation, true);
    this->setFocusPolicy(Qt::NoFocus);

    // 方案改进：直接调用内部非虚逻辑，彻底消除 VirtualCall 警告
    // 构造函数逻辑中不需要 invokeMethod，除非你需要等待父窗口布局完成
    mLayout.setDirection(QBoxLayout::LeftToRight);
    recalculateGeometryInternal();

    QWidget::hide();
}

SlidePanel::~SlidePanel() = default;

void SlidePanel::hide() {
    timeline.stop();
    QWidget::hide();
}

void SlidePanel::hideAnimated() {
    if (layoutManaged())
        hide();
    else if (!this->isHidden() && timeline.state() != QTimeLine::Running)
        timeline.start();
}

bool SlidePanel::layoutManaged() {
    return mLayoutManaged;
}

void SlidePanel::setLayoutManaged(bool mode) {
    mLayoutManaged = mode;
    if (!mode)
        recalculateGeometryInternal(); // 改用内部版本
}

// 优化：使用常量引用传递 shared_ptr
void SlidePanel::setWidget(const std::shared_ptr<QWidget>& w) {
    if (!w)
        return;
    if (hasWidget())
        layout()->removeWidget(mWidget.get());
    mWidget = w;
    mWidget->setParent(this);
    mLayout.insertWidget(0, mWidget.get());
}

bool SlidePanel::hasWidget() {
    return (mWidget != nullptr);
}

void SlidePanel::show() {
    if (hasWidget()) {
        timeline.stop();
        fadeEffect->setOpacity(panelVisibleOpacity);
        move(startPosition);
        QWidget::show();
        QWidget::raise();
    } else {
        qDebug() << "Warning: Trying to show panel containing no widget!";
    }
}

void SlidePanel::saveStaticGeometry(QRect geometry) {
    mStaticGeometry = geometry;
}

QRect SlidePanel::staticGeometry() {
    return mStaticGeometry;
}

void SlidePanel::animationUpdate(int /*frame*/) {
    // 使用 Qt 内部已经计算好的进度（避免重复计算）
    const qreal value = outCurve.valueForProgress(timeline.currentValue());

    // 仅在必要时检查鼠标（避免频繁 mapFromGlobal）
    const QPoint globalPos = QCursor::pos();
    const QPoint localPos = mapFromGlobal(globalPos);

    if (mTriggerRect.contains(localPos)) {
        timeline.stop();
        fadeEffect->setOpacity(panelVisibleOpacity);
        move(startPosition);
        return;
    }

    const QPoint delta = endPosition - startPosition;
    const QPoint newPos = startPosition + QPoint(
        static_cast<int>(delta.x() * value),
        static_cast<int>(delta.y() * value)
    );

    move(newPos);
    fadeEffect->setOpacity(1.0 - value);

    // ❌ 删除 qApp->processEvents(); （严重影响性能+可能引发递归）
}

void SlidePanel::setAnimationRange(QPoint start, QPoint end) {
    startPosition = start;
    endPosition = end;
}

void SlidePanel::onAnimationFinish() {
    QWidget::hide();
    fadeEffect->setOpacity(panelVisibleOpacity);
    move(startPosition);
}

QRect SlidePanel::triggerRect() {
    return mTriggerRect;
}

void SlidePanel::setPosition(PanelPosition p) {
    if (p == PANEL_TOP || p == PANEL_BOTTOM)
        mLayout.setDirection(QBoxLayout::LeftToRight);
    else
        mLayout.setDirection(QBoxLayout::BottomToTop);
    mPosition = p;
    recalculateGeometryInternal(); // 改用内部版本
}

PanelPosition SlidePanel::position() {
    return mPosition;
}

// 虚函数实现：简单封装内部逻辑
void SlidePanel::recalculateGeometry() {
    recalculateGeometryInternal();
}

// 核心计算逻辑：非虚函数，构造函数中可以安全调用
void SlidePanel::recalculateGeometryInternal() {
    if (layoutManaged())
        return;

    const QSize cSize = containerSize();
    const int w = width();
    const int h = height();

    switch (mPosition) {
    case PANEL_TOP:
        setAnimationRange(QPoint(0, 0), QPoint(0, -slideAmount));
        saveStaticGeometry(QRect(0, 0, cSize.width(), h));
        break;

    case PANEL_BOTTOM:
        setAnimationRange(QPoint(0, cSize.height() - h),
                          QPoint(0, cSize.height() - h + slideAmount));
        saveStaticGeometry(QRect(0, cSize.height() - h,
                                 cSize.width(), h));
        break;

    case PANEL_LEFT:
        setAnimationRange(QPoint(0, 0), QPoint(-slideAmount, 0));
        saveStaticGeometry(QRect(0, 0, w, cSize.height()));
        break;

    case PANEL_RIGHT:
    default:
        setAnimationRange(QPoint(cSize.width() - w, 0),
                          QPoint(cSize.width() - w + slideAmount, 0));
        saveStaticGeometry(QRect(cSize.width() - w, 0,
                                 w, cSize.height()));
        break;
    }

    setGeometry(mStaticGeometry);
    updateTriggerRectInternal();
}

void SlidePanel::updateTriggerRect() {
    updateTriggerRectInternal();
}

void SlidePanel::updateTriggerRectInternal() {
    mTriggerRect = mStaticGeometry;
}

void SlidePanel::setOrientation() {
    // 留空或实现
}