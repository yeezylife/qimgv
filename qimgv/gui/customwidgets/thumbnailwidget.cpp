#include "thumbnailwidget.h"
#include <QtMath>

ThumbnailWidget::ThumbnailWidget(QGraphicsItem *parent) :
    QGraphicsWidget(parent),
    thumbnail(nullptr)
{
}

ThumbnailWidget::~ThumbnailWidget() {
}

void ThumbnailWidget::updateDpr(qreal newDpr) {
    Q_UNUSED(newDpr)
}

void ThumbnailWidget::setThumbnailSize(int size) {
    Q_UNUSED(size)
}

void ThumbnailWidget::setPadding(int _padding) {
    Q_UNUSED(_padding)
}

void ThumbnailWidget::setMargins(int _marginX, int _marginY) {
    Q_UNUSED(_marginX)
    Q_UNUSED(_marginY)
}

int ThumbnailWidget::thumbnailSize() {
    return 0;
}

void ThumbnailWidget::reset() {
}

void ThumbnailWidget::setThumbStyle(ThumbnailStyle _style) {
    Q_UNUSED(_style)
}

void ThumbnailWidget::updateGeometry() {
    QGraphicsWidget::updateGeometry();
}

void ThumbnailWidget::setGeometry(const QRectF &rect) {
    QGraphicsWidget::setGeometry(rect);
}

QRectF ThumbnailWidget::geometry() const {
    return QGraphicsWidget::geometry();
}

QSizeF ThumbnailWidget::effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
    return QGraphicsWidget::effectiveSizeHint(which, constraint);
}

void ThumbnailWidget::setThumbnail(const std::shared_ptr<Thumbnail> &_thumbnail) {
    Q_UNUSED(_thumbnail)
}

void ThumbnailWidget::unsetThumbnail() {
}

void ThumbnailWidget::setupTextLayout() {
}

void ThumbnailWidget::updateBackgroundRect() {
}

void ThumbnailWidget::setHighlighted(bool mode) {
    Q_UNUSED(mode)
}

bool ThumbnailWidget::isHighlighted() {
    return false;
}

void ThumbnailWidget::setDropHovered(bool mode) {
    Q_UNUSED(mode)
}

bool ThumbnailWidget::isDropHovered() {
    return false;
}

QRectF ThumbnailWidget::boundingRect() const {
    return QRectF();
}

void ThumbnailWidget::updateBoundingRect() {
}

qreal ThumbnailWidget::width() {
    return 0;
}

qreal ThumbnailWidget::height() {
    return 0;
}

void ThumbnailWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void ThumbnailWidget::drawHighlight(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidget::drawHoverBg(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidget::drawHoverHighlight(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidget::drawLabel(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidget::drawSingleLineText(QPainter *painter, QRect rect, const QString &text, const QColor &color) {
    Q_UNUSED(painter)
    Q_UNUSED(rect)
    Q_UNUSED(text)
    Q_UNUSED(color)
}

void ThumbnailWidget::drawDropHover(QPainter *painter) {
    Q_UNUSED(painter)
}

void ThumbnailWidget::drawThumbnail(QPainter* painter, const QPixmap *pixmap) {
    Q_UNUSED(painter)
    Q_UNUSED(pixmap)
}

void ThumbnailWidget::drawIcon(QPainter* painter, const QPixmap *pixmap) {
    Q_UNUSED(painter)
    Q_UNUSED(pixmap)
}

QSizeF ThumbnailWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const {
    Q_UNUSED(which)
    Q_UNUSED(constraint)
    return QSizeF();
}

void ThumbnailWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsWidget::hoverEnterEvent(event);
}

void ThumbnailWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsWidget::hoverLeaveEvent(event);
}

void ThumbnailWidget::setHovered(bool mode) {
    Q_UNUSED(mode)
}

bool ThumbnailWidget::isHovered() {
    return false;
}

void ThumbnailWidget::updateThumbnailDrawPosition() {
}
