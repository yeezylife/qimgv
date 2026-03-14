#include "mapoverlay.h"
#include <QPropertyAnimation>
#include "settings.h"
#include <memory>

class MapOverlay::MapOverlayPrivate : public QObject {
public:
    MapOverlayPrivate(MapOverlay *qq);
    ~MapOverlayPrivate();
    void moveInnerWidget(float x, float y);
    void moveMainImage(float xPos, float yPos);

    QPen outlinePen;
    float xSpeedDiff, ySpeedDiff;
    QRectF outerRect, innerRect;
    QRectF windowRect, drawingRect;
    MapOverlay *q;
    int size;
    float opacity;
    float innerOffset;
    int margin;

    std::unique_ptr<QPropertyAnimation> opacityAnimation;
    std::unique_ptr<QPropertyAnimation> transitionAnimation;
    MapOverlay::Location location;
};

MapOverlay::MapOverlayPrivate::MapOverlayPrivate(MapOverlay *qq)
    : q(qq), size(120), opacity(0.0f), innerOffset(-1), margin(20) {
    outlinePen.setColor(QColor(180, 180, 180, 255));
    location = MapOverlay::RightBottom;
}

MapOverlay::MapOverlayPrivate::~MapOverlayPrivate() = default;

void MapOverlay::MapOverlayPrivate::moveInnerWidget(float x, float y) {
    if(x + static_cast<float>(innerRect.width()) > static_cast<float>(outerRect.right()))
        x = static_cast<float>(outerRect.right() - innerRect.width());

    if(x < 0)
        x = 0;

    if(y + static_cast<float>(innerRect.height()) > static_cast<float>(outerRect.bottom()))
        y = static_cast<float>(outerRect.bottom() - innerRect.height());

    if(y < 0)
        y = 0;

    innerRect.moveTo(QPointF(x, y));
    q->update();
}

void MapOverlay::MapOverlayPrivate::moveMainImage(float xPos, float yPos) {
    float x = static_cast<float>(xPos - (innerRect.width() / 2.0));
    float y = static_cast<float>(yPos - (innerRect.height() / 2.0));

    moveInnerWidget(x, y);

    x /= -xSpeedDiff;
    y /= -ySpeedDiff;

    // Check limits;
    float invisibleX = static_cast<float>(windowRect.width() - drawingRect.width());
    float invisibleY = static_cast<float>(windowRect.height() - drawingRect.height());

    if(x < invisibleX) x = invisibleX;
    if(x > 0) x = 0;

    if(y < invisibleY) y = invisibleY;
    if(y > 0) y = 0;

    emit q->positionChanged(x, y);
}

MapOverlay::MapOverlay(QWidget *parent) : QWidget(parent),
    visibilityEnabled(true),
    d(std::make_unique<MapOverlayPrivate>(this)) {
    this->setMouseTracking(true);
    d->opacityAnimation = std::make_unique<QPropertyAnimation>(this, "opacity");
    d->opacityAnimation->setEasingCurve(QEasingCurve::OutSine);
    d->opacityAnimation->setDuration(150);

    d->transitionAnimation = std::make_unique<QPropertyAnimation>(this, "y");
    d->transitionAnimation->setDuration(200);
    d->transitionAnimation->setEasingCurve(QEasingCurve::OutExpo);

    this->setVisible(true);
}

MapOverlay::~MapOverlay() = default;

QSizeF MapOverlay::inner() const {
    return d->innerRect.size();
}

QSizeF MapOverlay::outer() const {
    return d->outerRect.size();
}

float MapOverlay::opacity() const {
    return d->opacity;
}

void MapOverlay::enableVisibility(bool mode) {
    visibilityEnabled = mode;
}

void MapOverlay::setOpacity(float opacity) {
    d->opacity = opacity;
    update();
}

void MapOverlay::animateVisible(bool isVisible) {
    if(isVisible) this->setOpacity(1.0f);
    else {
        d->opacityAnimation->setEndValue(isVisible ? 1.0f : 0.0f);

        switch(location()) {
            case MapOverlay::LeftTop:
            case MapOverlay::RightTop:
                if(isVisible) {
                    d->transitionAnimation->setStartValue(0);
                    d->transitionAnimation->setEndValue(margin());
                } else {
                    d->transitionAnimation->setStartValue(margin());
                    d->transitionAnimation->setEndValue(0);
                }
                break;
            case MapOverlay::RightBottom:
            case MapOverlay::LeftBottom:
                int h = parentWidget()->height();
                int offset = static_cast<int>(d->outerRect.height() + margin());

                if(isVisible) {
                    d->transitionAnimation->setStartValue(h);
                    d->transitionAnimation->setEndValue(h - offset);
                } else {
                    d->transitionAnimation->setStartValue(h - offset);
                    d->transitionAnimation->setEndValue(h);
                }
                break;
        }

        d->opacityAnimation->start();
        //d->transitionAnimation->start();
    }
    //if (QWidget::isVisible() == isVisible) // already in this state
    //    return;
}

void MapOverlay::resize(int size) {
    QWidget::resize(size, size);
}

void MapOverlay::setY(int y) {
    move(x(), y);
}

int MapOverlay::y() const {
    return QWidget::y();
}

void MapOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)

    QPainter painter(this);
    QBrush outerBrush(QColor(40, 40, 40, 160), Qt::SolidPattern);
    QBrush innerBrush(QColor(180, 180, 180, 255), Qt::SolidPattern);

    painter.setOpacity(d->opacity);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter.fillRect(d->outerRect, outerBrush);
    painter.fillRect(d->innerRect, innerBrush);

    painter.setPen(d->outlinePen);
    painter.drawRect(d->outerRect);
}

void MapOverlay::updatePosition() {
    QRect parentRect = parentWidget()->rect();

    int x = 0, y = 0;
    switch(location()) {
        case MapOverlay::LeftTop:
            x = parentRect.left() + margin();
            y = parentRect.top() + margin();
            break;
        case MapOverlay::RightTop:
            x = static_cast<int>(parentRect.right() - (margin() + d->outerRect.width()));
            y = parentRect.top() + margin();
            break;
        case MapOverlay::RightBottom:
            x = static_cast<int>(parentRect.right() - (margin() + d->outerRect.width()));
            y = static_cast<int>(parentRect.bottom() - (margin() + d->outerRect.height()));
            break;
        case MapOverlay::LeftBottom:
            x = parentRect.left() + margin();
            y = static_cast<int>(parentRect.bottom() - (margin() + d->outerRect.height()));
            break;
    }

    /**
     * Save extra space for outer rect border
     * one pixel for right and bottom sides
     */
    setGeometry(x, y, d->size + 1, d->size + 1);
}

bool contains(const QRectF &real, const QRectF &expected) {
    return real.width() <= expected.width() && real.height() <= expected.height();
}

void MapOverlay::updateMap(const QRectF &drawingRect) {
    if(!isEnabled())
        return;

    QRectF windowRect = parentWidget()->rect();

    imageDoesNotFit = !contains(drawingRect, windowRect);
    animateVisible(imageDoesNotFit && visibilityEnabled);

    /**
     * Always calculate this first for properly map location
     */
    QSizeF outerSz = drawingRect.size();
    outerSz.scale(d->size, d->size, Qt::KeepAspectRatio);
    d->outerRect.setSize(outerSz);

    d->windowRect = windowRect;
    d->drawingRect = drawingRect;

    float aspect = static_cast<float>(outerSz.width() / drawingRect.width());

    float innerWidth = std::min(static_cast<float>(windowRect.width()) * aspect,
                                static_cast<float>(outerSz.width()));

    float innerHeight = std::min(static_cast<float>(windowRect.height()) * aspect,
                                 static_cast<float>(outerSz.height()));

    QSizeF innerSz(innerWidth, innerHeight);
    d->innerRect.setSize(innerSz);

    d->xSpeedDiff = static_cast<float>(innerSz.width() / windowRect.width());
    d->ySpeedDiff = static_cast<float>(innerSz.height() / windowRect.height());

    float x = static_cast<float>(-drawingRect.left() * d->xSpeedDiff);
    float y = static_cast<float>(-drawingRect.top() * d->ySpeedDiff);

    d->moveInnerWidget(x, y);
    update();
}

void MapOverlay::mousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    setCursor(Qt::ClosedHandCursor);
    d->moveMainImage(static_cast<float>(event->position().x()),
                     static_cast<float>(event->position().y()));
    event->accept();
}

void MapOverlay::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);

    if(event->buttons() & Qt::LeftButton) {
        d->moveMainImage(static_cast<float>(event->position().x()),
                         static_cast<float>(event->position().y()));
    }
    event->accept();
}

void MapOverlay::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
    setCursor(Qt::ArrowCursor);
    event->accept();
}

void MapOverlay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updatePosition();
}

void MapOverlay::leaveEvent(QEvent *event) {
    Q_UNUSED(event)
    this->enableVisibility(false);
    this->animateVisible(false);
    this->update();
}

void MapOverlay::enterEvent(QEnterEvent *event) {
    Q_UNUSED(event)
    this->enableVisibility(isVisible());
    this->animateVisible(visibilityEnabled && imageDoesNotFit);
    this->update();
}

int MapOverlay::size() const {
    return d->size;
}

MapOverlay::Location MapOverlay::location() const {
    return d->location;
}

void MapOverlay::setLocation(MapOverlay::Location loc) {
    d->location = loc;
}

int MapOverlay::margin() const {
    return d->margin;
}

void MapOverlay::setMargin(int margin) {
    d->margin = margin;
}