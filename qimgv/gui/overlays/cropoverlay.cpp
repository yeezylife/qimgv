#include "cropoverlay.h"

CropOverlay::CropOverlay(FloatingWidgetContainer *parent) 
    : FloatingWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dpr = devicePixelRatioF();
    handleSize = static_cast<int>(8 * dpr); // 保持 UI 元素比例

    if(parent) setContainerSize(parent->size());
    hide();
}

void CropOverlay::setImageRealSize(QSize sz) {
    imageRect = QRectF(QPointF(0, 0), QSizeF(sz));
    clearSelection();
}

void CropOverlay::setImageDrawRect(const QRect& rect) {
    imageDrawRect = rect; // Qt 6 默认处理逻辑像素
    updateSelectionDrawRect();
}

void CropOverlay::setImageScale(float s) {
    scale = s;
}

void CropOverlay::clearSelection() {
    if (hasSelection()) {
        selectionRect = QRectF();
        selectionDrawRect = QRectF();
        update();
        emit selectionChanged(QRect());
    }
}

bool CropOverlay::hasSelection() const {
    return selectionRect.isValid() && !selectionRect.isNull();
}

void CropOverlay::selectAll() {
    selectionRect = imageRect;
    updateSelectionDrawRect();
    update();
    emit selectionChanged(selectionRect.toRect());
}

void CropOverlay::setLockAspectRatio(bool mode) {
    lockAspectRatio = mode;
}

void CropOverlay::setAspectRatio(const QPointF& ratio) {
    if (qFuzzyIsNull(ratio.x()) || qFuzzyIsNull(ratio.y())) return;
    aspectRatio = ratio;
    setLockAspectRatio(true);
    if (hasSelection()) {
        resizeSelection(QPointF(0, 0)); // 强制应用比例
        emit selectionChanged(selectionRect.toRect());
    }
}

void CropOverlay::hide() {
    clearSelection();
    FloatingWidget::hide();
}

//------------------------------------------------------------------------------
// 渲染优化：四矩形填充法
//------------------------------------------------------------------------------
void CropOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false); // 选区线条不需要抗锯齿，保持锐利

    if (!hasSelection()) {
        p.fillRect(imageDrawRect, brushInactiveTint);
        return;
    }

    // 1. 绘制四个半透明遮罩（上、下、左、右）替代 QRegion，性能极高
    p.setPen(Qt::NoPen);
    // Top
    p.fillRect(QRectF(imageDrawRect.left(), imageDrawRect.top(), imageDrawRect.width(), selectionDrawRect.top() - imageDrawRect.top()), brushInactiveTint);
    // Bottom
    p.fillRect(QRectF(imageDrawRect.left(), selectionDrawRect.bottom(), imageDrawRect.width(), imageDrawRect.bottom() - selectionDrawRect.bottom()), brushInactiveTint);
    // Left
    p.fillRect(QRectF(imageDrawRect.left(), selectionDrawRect.top(), selectionDrawRect.left() - imageDrawRect.left(), selectionDrawRect.height()), brushInactiveTint);
    // Right
    p.fillRect(QRectF(selectionDrawRect.right(), selectionDrawRect.top(), imageDrawRect.right() - selectionDrawRect.right(), selectionDrawRect.height()), brushInactiveTint);

    // 2. 绘制选区边框
    p.setPen(selectionOutlinePen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(selectionDrawRect);

    // 3. 绘制手柄
    if (cursorAction == CursorAction::None && selectionDrawRect.width() > 40) {
        p.setBrush(brushHandle);
        for (const auto& handle : handles) {
            p.drawRect(handle);
        }
    }
}

//------------------------------------------------------------------------------
// 几何计算逻辑
//------------------------------------------------------------------------------
void CropOverlay::updateSelectionDrawRect() {
    if (!hasSelection()) return;
    
    // 图片坐标 -> Widget 逻辑坐标
    selectionDrawRect.setTopLeft(selectionRect.topLeft() * scale + imageDrawRect.topLeft());
    selectionDrawRect.setSize(selectionRect.size() * scale);
    updateHandlePositions();
}

void CropOverlay::updateHandlePositions() {
    const qreal hs = 6.0; // 逻辑像素手柄大小
    const QRectF& r = selectionDrawRect;
    
    handles[0] = QRectF(r.left() - hs, r.top() - hs, hs*2, hs*2);       // TL
    handles[1] = QRectF(r.right() - hs, r.top() - hs, hs*2, hs*2);      // TR
    handles[2] = QRectF(r.left() - hs, r.bottom() - hs, hs*2, hs*2);    // BL
    handles[3] = QRectF(r.right() - hs, r.bottom() - hs, hs*2, hs*2);   // BR
    handles[4] = QRectF(r.left() - hs, r.center().y() - hs, hs*2, hs*2);// L
    handles[5] = QRectF(r.right() - hs, r.center().y() - hs, hs*2, hs*2);// R
    handles[6] = QRectF(r.center().x() - hs, r.top() - hs, hs*2, hs*2); // T
    handles[7] = QRectF(r.center().x() - hs, r.bottom() - hs, hs*2, hs*2);// B
}

QPointF CropOverlay::mapToImage(const QPointF& widgetPos) const {
    QPointF p = (widgetPos - imageDrawRect.topLeft()) / scale;
    return QPointF(std::clamp(p.x(), 0.0, imageRect.width()),
                   std::clamp(p.y(), 0.0, imageRect.height()));
}

//------------------------------------------------------------------------------
// 交互逻辑重构
//------------------------------------------------------------------------------
void CropOverlay::resizeSelection(const QPointF& delta) {
    QRectF newRect = selectionRect;

    // 1. 基础位移处理
    switch (cursorAction) {
        case CursorAction::DragTopLeft:     newRect.setTopLeft(newRect.topLeft() + delta); break;
        case CursorAction::DragTopRight:    newRect.setTopRight(newRect.topRight() + delta); break;
        case CursorAction::DragBottomLeft:  newRect.setBottomLeft(newRect.bottomLeft() + delta); break;
        case CursorAction::DragBottomRight: newRect.setBottomRight(newRect.bottomRight() + delta); break;
        case CursorAction::DragLeft:        newRect.setLeft(newRect.left() + delta.x()); break;
        case CursorAction::DragRight:       newRect.setRight(newRect.right() + delta.x()); break;
        case CursorAction::DragTop:         newRect.setTop(newRect.top() + delta.y()); break;
        case CursorAction::DragBottom:      newRect.setBottom(newRect.bottom() + delta.y()); break;
        default: break;
    }

    // 2. 等比缩放限制
    if (lockAspectRatio) {
        qreal targetRatio = aspectRatio.x() / aspectRatio.y();
        QSizeF sz = newRect.size();
        sz.scale(sz.width(), sz.width() / targetRatio, Qt::KeepAspectRatio);
        newRect.setSize(sz);
        
        // 保持锚点不动
        if (cursorAction == CursorAction::DragTopLeft) newRect.moveBottomRight(resizeAnchor);
        else if (cursorAction == CursorAction::DragTopRight) newRect.moveBottomLeft(resizeAnchor);
        else if (cursorAction == CursorAction::DragBottomLeft) newRect.moveTopRight(resizeAnchor);
        else newRect.moveTopLeft(resizeAnchor);
    }

    // 3. 边界归一化与约束
    selectionRect = newRect.normalized().intersected(imageRect);
    updateSelectionDrawRect();
}

CursorAction CropOverlay::hoverTarget(const QPointF& pos) const {
    if (handles[0].contains(pos)) return CursorAction::DragTopLeft;
    if (handles[1].contains(pos)) return CursorAction::DragTopRight;
    if (handles[2].contains(pos)) return CursorAction::DragBottomLeft;
    if (handles[3].contains(pos)) return CursorAction::DragBottomRight;
    if (handles[4].contains(pos)) return CursorAction::DragLeft;
    if (handles[5].contains(pos)) return CursorAction::DragRight;
    if (handles[6].contains(pos)) return CursorAction::DragTop;
    if (handles[7].contains(pos)) return CursorAction::DragBottom;
    if (selectionDrawRect.contains(pos)) return CursorAction::DragMove;
    return CursorAction::None;
}

void CropOverlay::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        moveStartPos = event->position();
        cursorAction = hoverTarget(moveStartPos);
        
        if (!hasSelection()) {
            cursorAction = CursorAction::SelectionStart;
            selectionRect.setTopLeft(mapToImage(moveStartPos));
            selectionRect.setBottomRight(selectionRect.topLeft());
        }
        
        setResizeAnchorByAction(cursorAction);
        setCursorByAction(cursorAction);
    } else if (event->button() == Qt::RightButton) {
        clearSelection();
    }
}

void CropOverlay::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        QPointF currentPos = event->position();
        QPointF delta = (currentPos - moveStartPos) / scale;
        
        if (cursorAction == CursorAction::DragMove) {
            selectionRect.translate(delta);
            // 限制在图片范围内
            if (selectionRect.left() < 0) selectionRect.moveLeft(0);
            if (selectionRect.right() > imageRect.right()) selectionRect.moveRight(imageRect.right());
            if (selectionRect.top() < 0) selectionRect.moveTop(0);
            if (selectionRect.bottom() > imageRect.bottom()) selectionRect.moveBottom(imageRect.bottom());
            updateSelectionDrawRect();
        } else {
            resizeSelection(delta);
        }
        
        moveStartPos = currentPos;
        update();
        emit selectionChanged(selectionRect.toRect());
    } else {
        setCursorByAction(hoverTarget(event->position()));
    }
}

void CropOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (cursorAction == CursorAction::SelectionStart && selectionRect.size().isEmpty()) {
        clearSelection();
    }
    cursorAction = CursorAction::None;
    setCursorByAction(hoverTarget(event->position()));
    update();
}

void CropOverlay::setCursorByAction(CursorAction action) {
    switch (action) {
        case CursorAction::DragTopLeft:     
        case CursorAction::DragBottomRight: setCursor(Qt::SizeFDiagCursor); break;
        case CursorAction::DragTopRight:    
        case CursorAction::DragBottomLeft:  setCursor(Qt::SizeBDiagCursor); break;
        case CursorAction::DragLeft:        
        case CursorAction::DragRight:       setCursor(Qt::SizeHorCursor); break;
        case CursorAction::DragTop:         
        case CursorAction::DragBottom:      setCursor(Qt::SizeVerCursor); break;
        case CursorAction::DragMove:        setCursor(Qt::OpenHandCursor); break;
        default:                            setCursor(Qt::ArrowCursor); break;
    }
}

void CropOverlay::setResizeAnchorByAction(CursorAction action) {
    switch (action) {
        case CursorAction::DragTopLeft:     resizeAnchor = selectionRect.bottomRight(); break;
        case CursorAction::DragTopRight:    resizeAnchor = selectionRect.bottomLeft(); break;
        case CursorAction::DragBottomLeft:  resizeAnchor = selectionRect.topRight(); break;
        case CursorAction::DragBottomRight: 
        case CursorAction::SelectionStart:  resizeAnchor = selectionRect.topLeft(); break;
        case CursorAction::DragLeft:        resizeAnchor = selectionRect.topRight(); break;
        case CursorAction::DragRight:       resizeAnchor = selectionRect.topLeft(); break;
        case CursorAction::DragTop:         resizeAnchor = selectionRect.bottomLeft(); break;
        case CursorAction::DragBottom:      resizeAnchor = selectionRect.topLeft(); break;
        default: break;
    }
}

void CropOverlay::onSelectionOutsideChange(const QRect& selection) {
    if (selection.isValid()) {
        selectionRect = QRectF(selection).intersected(imageRect);
        updateSelectionDrawRect();
        update();
    }
}

void CropOverlay::keyPressEvent(QKeyEvent *event) {
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && hasSelection()) {
        (event->modifiers() == Qt::ShiftModifier) ? emit cropSave() : emit cropDefault();
    } else if (event->key() == Qt::Key_Escape) {
        clearSelection();
        emit escPressed();
    } else if (event->matches(QKeySequence::SelectAll)) {
        selectAll();
    } else {
        event->ignore();
    }
}

void CropOverlay::resizeEvent(QResizeEvent *event) {
    updateSelectionDrawRect();
    FloatingWidget::resizeEvent(event);
}

void CropOverlay::recalculateGeometry() {
    setGeometry(0, 0, containerSize().width(), containerSize().height());
}