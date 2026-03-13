#include "iconwidget.h"

IconWidget::IconWidget(QWidget *parent)
    : QWidget(parent)
{
    dpr = devicePixelRatioF();
    color = settings->colorScheme().icons;
    connect(settings, &Settings::settingsChanged, this, &IconWidget::onSettingsChanged);
}

IconWidget::~IconWidget() = default; // std::unique_ptr 会自动释放内存

void IconWidget::onSettingsChanged() {
    if(colorMode == ICON_COLOR_THEME && color != settings->colorScheme().icons) {
        color = settings->colorScheme().icons;
        applyColor();
    }
}

void IconWidget::setIconPath(const QString &path) {
    if(iconPath == path)
        return;
    iconPath = path;
    loadIcon();
}

void IconWidget::loadIcon() {
    auto path = iconPath;
    pixmap.reset(); // 释放旧对象
    
    if(dpr >= (1.0 + 0.001)) {
        path.replace(".", "@2x.");
        hiResPixmap = true;
        pixmap = std::make_unique<QPixmap>(path);
        if(dpr >= (2.0 - 0.001))
            pixmapDrawScale = dpr;
        else
            pixmapDrawScale = 2.0;
        pixmap->setDevicePixelRatio(pixmapDrawScale);
    } else {
        hiResPixmap = false;
        pixmap = std::make_unique<QPixmap>(path);
        pixmapDrawScale = dpr;
    }
    
    if(pixmap->isNull()) {
        pixmap.reset();
    } else {
        applyColor();
    }
    update();
}

QSize IconWidget::minimumSizeHint() const {
    if(pixmap && !pixmap->isNull())
        return pixmap->size() / dpr;
    
    return QWidget::minimumSizeHint();
}

void IconWidget::setIconOffset(const QPoint &offset) {
    iconOffset = offset;
    update();
}

void IconWidget::setIconOffset(int x, int y) {
    setIconOffset(QPoint(x, y));
}

void IconWidget::setColorMode(IconColorMode mode) {
    if(colorMode != mode && mode == ICON_COLOR_SOURCE) {
        colorMode = mode;
        loadIcon();
    } else {
        colorMode = mode;
        applyColor();
    }
}

void IconWidget::setColor(QColor color) {
    this->colorMode = ICON_COLOR_CUSTOM;
    this->color = color;
    applyColor();
}

void IconWidget::applyColor() {
    if(!pixmap || pixmap->isNull() || colorMode == ICON_COLOR_SOURCE)
        return;
    ImageLib::recolor(*pixmap, color);
}

void IconWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QPainter p(this);
    if(!isEnabled())
        p.setOpacity(0.5f);

    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    if(pixmap) {
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QPointF pos;
        // 修复 bugprone-integer-division：使用 2.0 强制进入浮点运算
        if(hiResPixmap) {
            pos = QPointF(width() / 2.0 - pixmap->width() / (2.0 * pixmapDrawScale),
                          height() / 2.0 - pixmap->height() / (2.0 * pixmapDrawScale));
        } else {
            pos = QPointF(width() / 2.0 - pixmap->width() / 2.0,
                          height() / 2.0 - pixmap->height() / 2.0);
        }
        p.drawPixmap(pos + iconOffset, *pixmap);
    }
}