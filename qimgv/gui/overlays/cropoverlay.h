#pragma once
#include "gui/customwidgets/floatingwidget.h"
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <algorithm>

enum class CursorAction {
    None,
    SelectionStart,
    DragMove,
    DragLeft,
    DragRight,
    DragTop,
    DragBottom,
    DragTopLeft,
    DragTopRight,
    DragBottomLeft,
    DragBottomRight
};

class CropOverlay : public FloatingWidget
{
    Q_OBJECT
public:
    explicit CropOverlay(FloatingWidgetContainer *parent = nullptr);
    
    void setImageDrawRect(const QRect& rect);
    void setImageRealSize(const QSize& sz);
    void setImageScale(float s);
    void clearSelection();

signals:
    void selectionChanged(const QRect& rect);
    void escPressed();
    void cropDefault();
    void cropSave();

public slots:
    void hide();
    void onSelectionOutsideChange(const QRect& selection);
    void selectAll();
    void setAspectRatio(const QPointF& ratio);
    void setLockAspectRatio(bool mode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // 核心几何数据 (逻辑坐标/图片坐标)
    QRectF imageRect;         // 原图全尺寸 QRect
    QRectF selectionRect;     // 选区在原图上的位置 (浮点精度防止缩放抖动)
    QRectF imageDrawRect;     // 图像在 Widget 上的显示区域 (逻辑像素)
    QRectF selectionDrawRect; // 选区在 Widget 上的显示区域 (逻辑像素)
    
    // 状态与配置
    QPointF moveStartPos;
    QPointF resizeAnchor;
    QPointF aspectRatio{16.0, 9.0};
    float scale = 1.0f;
    qreal dpr = 1.0;
    bool lockAspectRatio = false;
    CursorAction cursorAction = CursorAction::None;

    // 绘制资源
    int handleSize = 8;
    QRectF handles[8];
    QBrush brushInactiveTint{QColor(0, 0, 0, 160)};
    QBrush brushHandle{QColor(150, 150, 150, 160)};
    QPen selectionOutlinePen{Qt::white, 1.0};

    // 私有辅助工具
    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] CursorAction hoverTarget(const QPointF& pos) const;
    void updateSelectionDrawRect();
    void updateHandlePositions();
    void setCursorByAction(CursorAction action);
    void setResizeAnchorByAction(CursorAction action);
    void resizeSelection(const QPointF& delta);
    void applyAspectRatio();                              // 将现有选区调整为锁定比例
    QPointF adjustPointForAspectRatio(const QPointF& anchor, const QPointF& point, qreal ratio) const; // 新选区时调整鼠标点
    
    // 坐标映射
    [[nodiscard]] QPointF mapToImage(const QPointF& widgetPos) const;
    void recalculateGeometry() override;
};