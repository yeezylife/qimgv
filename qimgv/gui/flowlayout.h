#pragma once

#include <QGraphicsLayout>

struct GridInfo {
    GridInfo(int _columns, int _rows, qreal _height) {
        columns = _columns;
        rows = _rows;
        height = _height;
    }
    int columns, rows;
    qreal height;
};

class FlowLayout : public QGraphicsLayout
{
public:
    FlowLayout();
    void insertItem(int index, QGraphicsLayoutItem *item);
    void setSpacing(Qt::Orientations o, qreal spacing);
    qreal spacing(Qt::Orientation o) const;

    // inherited functions
    void setGeometry(const QRectF &geom) override;

    int count() const override;
    QGraphicsLayoutItem *itemAt(int index) const override;
    void removeAt(int index) override;

    // returns the index of item above / below
    int itemAbove(int index);
    int itemBelow(int index);
    int rows();
    int columns();
    void clear();

    int columnOf(int index);
    bool sameRow(int one, int two);

protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;

private:
    GridInfo doLayout(const QRectF &geom, bool applyNewGeometry) const;
    QSizeF minSize(const QSizeF &constraint) const;
    QSizeF prefSize() const;
    QSizeF maxSize() const;
    void invalidateSizeCache() const;

    QList<QGraphicsLayoutItem*> m_items;
    qreal m_spacing[2];
    int m_rows, m_columns;
};
