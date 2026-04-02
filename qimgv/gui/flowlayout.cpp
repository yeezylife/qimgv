#include "flowlayout.h"

void FlowLayout::invalidateSizeCache() const
{
}

int FlowLayout::itemAbove(int index) {
    Q_UNUSED(index)
    return -1;
}

int FlowLayout::itemBelow(int index) {
    Q_UNUSED(index)
    return -1;
}

bool FlowLayout::sameRow(int one, int two) {
    Q_UNUSED(one)
    Q_UNUSED(two)
    return false;
}

int FlowLayout::columnOf(int index) {
    Q_UNUSED(index)
    return -1;
}

int FlowLayout::rows() {
    return 0;
}

int FlowLayout::columns() {
    return 0;
}

void FlowLayout::insertItem(int index, QGraphicsLayoutItem *item) {
    Q_UNUSED(index)
    Q_UNUSED(item)
}

int FlowLayout::count() const
{
    return 0;
}

QGraphicsLayoutItem *FlowLayout::itemAt(int index) const
{
    Q_UNUSED(index)
    return nullptr;
}

void FlowLayout::removeAt(int index)
{
    Q_UNUSED(index)
}

void FlowLayout::clear()
{
}

qreal FlowLayout::spacing(Qt::Orientation o) const
{
    Q_UNUSED(o)
    return 0;
}

void FlowLayout::setSpacing(Qt::Orientations o, qreal spacing)
{
    Q_UNUSED(o)
    Q_UNUSED(spacing)
}

void FlowLayout::setGeometry(const QRectF &geom)
{
    Q_UNUSED(geom)
}

GridInfo FlowLayout::doLayout(const QRectF &geom, bool applyNewGeometry) const {
    Q_UNUSED(geom)
    Q_UNUSED(applyNewGeometry)
    return GridInfo(0, 0, 0);
}

QSizeF FlowLayout::minSize(const QSizeF &constraint) const
{
    Q_UNUSED(constraint)
    return QSizeF();
}

QSizeF FlowLayout::prefSize() const
{
    return QSizeF();
}

QSizeF FlowLayout::maxSize() const
{
    return QSizeF();
}

QSizeF FlowLayout::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    Q_UNUSED(which)
    Q_UNUSED(constraint)
    return QSizeF();
}