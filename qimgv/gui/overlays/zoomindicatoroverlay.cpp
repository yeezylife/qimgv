#include "zoomindicatoroverlay.h"
#include <QPainter>

ZoomIndicatorOverlay::ZoomIndicatorOverlay(FloatingWidgetContainer *parent)
    : OverlayWidget(parent),
      m_fm(font()),
      m_cachedFont(font())
{
    visibilityTimer.setSingleShot(true);
    visibilityTimer.setInterval(hideDelay);

    setPosition(FloatingWidgetPosition::BOTTOMLEFT);
    setHorizontalMargin(0);
    setVerticalMargin(32);

    setFadeEnabled(true);
    setFadeDuration(300);

    connect(&visibilityTimer, &QTimer::timeout,
            this, &ZoomIndicatorOverlay::hideAnimated);

    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    if (parent)
        setContainerSize(parent->size());
}

void ZoomIndicatorOverlay::updateCache()
{
    if (m_text.isEmpty())
        return;
    // 字体不变时复用度量，仅文本变化时重算宽度；字体变化（HiDPI/样式表）后重建
    const QFont cur = font();
    if (m_cachedFont != cur) {
        m_fm = QFontMetrics(cur);
        m_cachedFont = cur;
    }
    m_textWidth = m_fm.horizontalAdvance(m_text);
    m_ascent = m_fm.ascent();
    m_descent = m_fm.descent();
    int totalHeight = m_ascent + m_descent;
    setFixedSize(m_textWidth + 16, totalHeight + 12);
}

void ZoomIndicatorOverlay::setScale(qreal scale)
{
    const QString newText = QString::number(qRound(scale * 100.0)) + '%';
    // 同值重复触发（resize silent 路径）直接返回，避免度量/布局/重绘
    if (newText == m_text && m_cachedFont == font())
        return;
    m_text = newText;

    updateCache();      // 一次性计算所有尺寸
    recalculateGeometry();
    update();           // 触发重绘
}

void ZoomIndicatorOverlay::show(int duration)
{
    visibilityTimer.stop();
    OverlayWidget::show();
    visibilityTimer.setInterval(duration);
    visibilityTimer.start();
}

void ZoomIndicatorOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_text.isEmpty())
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);   // 只开文本抗锯齿
    p.setFont(font());

    int x = (width() - m_textWidth) / 2;
    int y = (height() - (m_ascent + m_descent)) / 2 + m_ascent;

    static const QColor shadowColor(0, 0, 0, 255);   // 避免重复构造
    p.setPen(shadowColor);
    p.drawText(x + 1, y + 1, m_text);   // 右下阴影

    p.setPen(Qt::white);
    p.drawText(x, y, m_text);
}