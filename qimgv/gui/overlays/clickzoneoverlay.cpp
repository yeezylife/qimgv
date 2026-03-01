#include "clickzoneoverlay.h"

ClickZoneOverlay::ClickZoneOverlay(FloatingWidgetContainer *parent)
    : FloatingWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    if (parent)
        setContainerSize(parent->size());

    dpr = devicePixelRatioF();

    pixmapLeft = loadPixmap(":/res/icons/common/overlay/arrow_left_50.png");
    pixmapRight = loadPixmap(":/res/icons/common/overlay/arrow_right_50.png");

    connect(settings, &Settings::settingsChanged, this, &ClickZoneOverlay::readSettings);
    readSettings();

    show();
}

void ClickZoneOverlay::readSettings() {
    bool newDrawZones = settings->clickableEdgesVisible();
    if (drawZones == newDrawZones)
        return;
    drawZones = newDrawZones;
    update();
}

QPixmap ClickZoneOverlay::loadPixmap(const QString& path) {
    QString actualPath = path;
    QPixmap pixmap;
    if (dpr >= 1.001) {
        actualPath.replace(".", "@2x.");
        pixmap.load(actualPath);
        hiResPixmaps = true;
        pixmapDrawScale = (dpr >= 1.999) ? dpr : 2.0;
        pixmap.setDevicePixelRatio(pixmapDrawScale);
    } else {
        hiResPixmaps = false;
        pixmap.load(actualPath);
        pixmapDrawScale = dpr;
    }
    if (!pixmap.isNull()) {
        ImageLib::recolor(pixmap, QColor(255, 255, 255));
    }
    return pixmap;
}

QRect ClickZoneOverlay::leftZone() const {
    return mLeftZone;
}

QRect ClickZoneOverlay::rightZone() const {
    return mRightZone;
}

void ClickZoneOverlay::highlightLeft() {
    setHighlightedZone(HIGHLIGHT_LEFT);
}

void ClickZoneOverlay::highlightRight() {
    setHighlightedZone(HIGHLIGHT_RIGHT);
}

void ClickZoneOverlay::disableHighlight() {
    setHighlightedZone(HIGHLIGHT_NONE);
}

bool ClickZoneOverlay::isHighlighted() const {
    return activeZone != HIGHLIGHT_NONE;
}

void ClickZoneOverlay::setPressed(bool mode) {
    if (isPressed == mode)
        return;
    isPressed = mode;
    if (isHighlighted())
        update();
}

void ClickZoneOverlay::setHighlightedZone(ActiveHighlightZone zone) {
    activeZone = zone;
    update();
}

void ClickZoneOverlay::recalculateGeometry() {
    setGeometry(0, 0, containerSize().width(), containerSize().height());
}

void ClickZoneOverlay::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    mLeftZone = QRect(0, 0, zoneSize, height());
    mRightZone = QRect(width() - zoneSize, 0, zoneSize, height());
}

void ClickZoneOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    if (activeZone == HIGHLIGHT_NONE || !drawZones || width() <= 250)
        return;
    QPainter p(this);
    p.setOpacity(isPressed ? 0.06f : 0.10f);
    QBrush brush(QColor(200, 200, 200), Qt::SolidPattern);

    if (activeZone == HIGHLIGHT_LEFT) {
        p.fillRect(mLeftZone, brush);
        drawPixmap(p, pixmapLeft, mLeftZone);
    }
    if (activeZone == HIGHLIGHT_RIGHT) {
        p.fillRect(mRightZone, brush);
        drawPixmap(p, pixmapRight, mRightZone);
    }
}

void ClickZoneOverlay::drawPixmap(QPainter &p, const QPixmap& pixmap, const QRect& rect) {
    p.setOpacity(isPressed ? 0.37f : 0.5f);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QPointF pos;
    if (hiResPixmaps) {
        pos = QPointF(rect.left() + rect.width()  / 2 - pixmap.width()  / (2 * pixmapDrawScale),
                      rect.top()  + rect.height() / 2 - pixmap.height() / (2 * pixmapDrawScale));
    } else {
        pos = QPointF(rect.left() + rect.width()  / 2 - pixmap.width()  / 2,
                      rect.top()  + rect.height() / 2 - pixmap.height() / 2);
    }
    p.drawPixmap(pos, pixmap);
}