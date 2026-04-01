#include "ThumbnailWidgetCmp.h"

ThumbnailWidgetCmp::ThumbnailWidgetCmp(QGraphicsItem *parent) :
    QGraphicsWidget(parent),
    isLoaded(false),
    thumbnail(nullptr),
    highlighted(false),
    hovered(false),
    dropHovered(false),
    mThumbnailSize(100),
    padding(5),
    marginX(2),
    marginY(2),
    labelSpacing(9),
    textHeight(5),
    thumbStyle(THUMB_SIMPLE)
{
}

ThumbnailWidgetCmp::~ThumbnailWidgetCmp() {
}

void ThumbnailWidgetCmp::setThumbnailSize(int size) {
    Q_UNUSED(size)
}

void ThumbnailWidgetCmp::setPadding(int _padding) {
    Q_UNUSED(_padding)
}

void ThumbnailWidgetCmp::setMargins(int _marginX, int _marginY) {
    Q_UNUSED(_marginX)
    Q_UNUSED(_marginY)
}

int ThumbnailWidgetCmp::thumbnailSize() {
    return 0;
}

void ThumbnailWidgetCmp::reset() {
}

void ThumbnailWidgetCmp::setThumbStyle(ThumbnailStyle _style) {
    Q_UNUSED(_style)
}

void ThumbnailWidgetCmp::updateGeometry() {
    QGraphicsWidget::updateGeometry();
}

void ThumbnailWidgetCmp::setGeometry(const QRectF &rect) {
    QGraphicsWidget::setGeometry(rect);
}

QRectF ThumbnailWidgetCmp::geometry() const {
    return QGraphicsWidget::geometry();
}

QSizeF ThumbnailWidgetCmp::effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
    return QGraphicsWidget::effectiveSizeHint(which, constraint);
}

void ThumbnailWidgetCmp::setThumbnail(std::shared_ptr<Thumbnail> _thumbnail) {
    Q_UNUSED(_thumbnail)
}

void ThumbnailWidgetCmp::unsetThumbnail() {
}

void ThumbnailWidgetCmp::setupTextLayout() {
}

void ThumbnailWidgetCmp::updateBackgroundRect() {
}

void ThumbnailWidgetCmp::setHighlighted(bool mode) {
    Q_UNUSED(mode)
}

bool ThumbnailWidgetCmp::isHighlighted() {
    return false;
}

void ThumbnailWidgetCmp::setDropHovered(bool mode) {
    Q_UNUSED(mode)
}

bool ThumbnailWidgetCmp::isDropHovered() {
    return false;
}

QRectF ThumbnailWidgetCmp::boundingRect() const {
    return QRectF();
}

void ThumbnailWidgetCmp::updateBoundingRect() {
}

qreal ThumbnailWidgetCmp::width() {
    return 0;
}

qreal ThumbnailWidgetCmp::height() {
    return 0;
}

void ThumbnailWidgetCmp::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void ThumbnailWidgetCmp::drawHighlight(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidgetCmp::drawHoverBg(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidgetCmp::drawHoverHighlight(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidgetCmp::drawLabel(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidgetCmp::drawSingleLineText(QPainter *painter, QFont &_fnt, QRect rect, QString text, const QColor &color, bool center) {
    Q_UNUSED(painter)
    Q_UNUSED(_fnt)
    Q_UNUSED(rect)
    Q_UNUSED(text)
    Q_UNUSED(color)
    Q_UNUSED(center)
}

void ThumbnailWidgetCmp::drawDropHover(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidgetCmp::drawThumbnail(QPainter* painter, const QPixmap *pixmap) {
    Q_UNUSED(painter)
    Q_UNUSED(pixmap)
}

void ThumbnailWidgetCmp::drawIcon(QPainter* painter, const QPixmap *pixmap) {
    Q_UNUSED(painter)
    Q_UNUSED(pixmap)
}

QSizeF ThumbnailWidgetCmp::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
    Q_UNUSED(which)
    Q_UNUSED(constraint)
    return QSizeF();
}

void ThumbnailWidgetCmp::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsWidget::hoverEnterEvent(event);
}

void ThumbnailWidgetCmp::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsWidget::hoverLeaveEvent(event);
}

void ThumbnailWidgetCmp::setHovered(bool mode) {
    Q_UNUSED(mode)
}

bool ThumbnailWidgetCmp::isHovered() {
    return false;
}

void ThumbnailWidgetCmp::updateThumbnailDrawPosition() {
}
