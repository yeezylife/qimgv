#include "cropoverlay.h"
#include <QtMath>
#include <QRegion>

CropOverlay::CropOverlay(FloatingWidgetContainer *parent) 
    : FloatingWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dpr = devicePixelRatioF();
    handleSize = static_cast<int>(8 * dpr);

    if(parent) setContainerSize(parent->size());
    hide();
}

void CropOverlay::setImageRealSize(const QSize& sz) {
    imageRect = QRectF(QPointF(0, 0), QSizeF(sz));
    clearSelection();
}

void CropOverlay::setImageDrawRect(const QRect& rect) {
    imageDrawRect = rect;
    updateSelectionDrawRect();
}

void CropOverlay::setImageScale(float s) {
    scale = s;
}

void CropOverlay::clearSelection() {
    if (hasSelection()) {
        // 记录旧区域用于局部刷新
        QRectF oldDrawRect = selectionDrawRect;
        selectionRect = QRectF();
        selectionDrawRect = QRectF();
        updateHandlePositions();
        update(); // 因为整个区域都要刷新，直接全刷新
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
    if (lockAspectRatio && hasSelection()) {
        applyAspectRatio();
    }
}

void CropOverlay::setAspectRatio(const QPointF& ratio) {
    if (qFuzzyIsNull(ratio.x()) || qFuzzyIsNull(ratio.y())) return;
    aspectRatio = ratio;
    cachedRatio = aspectRatio.x() / aspectRatio.y(); // 优化2：缓存比例
    setLockAspectRatio(true);
    if (hasSelection()) {
        applyAspectRatio();
    }
}

void CropOverlay::hide() {
    clearSelection();
    FloatingWidget::hide();
}

//------------------------------------------------------------------------------
// 渲染
//------------------------------------------------------------------------------
void CropOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    if (!hasSelection()) {
        p.fillRect(imageDrawRect, brushInactiveTint);
        return;
    }

    // 优化5：使用QRegion合并填充外部区域
    QRegion outside(imageDrawRect.toRect());
    QRegion inside(selectionDrawRect.toRect());
    p.setClipRegion(outside.subtracted(inside));
    p.fillRect(imageDrawRect, brushInactiveTint);
    p.setClipping(false);

    p.setPen(selectionOutlinePen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(selectionDrawRect);

    if (cursorAction == CursorAction::None && selectionDrawRect.width() > 40) {
        p.setBrush(brushHandle);
        for (const auto& handle : drawHandles) {
            p.drawRect(handle);
        }
    }
}

//------------------------------------------------------------------------------
// 几何计算
//------------------------------------------------------------------------------
void CropOverlay::updateSelectionDrawRect() {
    if (!hasSelection()) return;
    // 记录旧区域用于局部刷新
    oldSelectionDrawRect = selectionDrawRect;
    selectionDrawRect.setTopLeft(selectionRect.topLeft() * scale + imageDrawRect.topLeft());
    selectionDrawRect.setSize(selectionRect.size() * scale);
    updateHandlePositions();
    
    // 优化1：局部刷新
    if (!oldSelectionDrawRect.isNull() || !selectionDrawRect.isNull()) {
        QRect dirty = oldSelectionDrawRect.toAlignedRect().adjusted(-handleSize, -handleSize, handleSize, handleSize);
        dirty |= selectionDrawRect.toAlignedRect().adjusted(-handleSize, -handleSize, handleSize, handleSize);
        update(dirty);
    } else {
        update(); // fallback
    }
}

void CropOverlay::updateHandlePositions() {
    const qreal drawSize = handleSize * 0.75;   // 绘制区域较小
    const qreal hitSize = handleSize;           // 命中区域较大
    const qreal drawHalf = drawSize * 0.5;
    const qreal hitHalf = hitSize * 0.5;
    const QRectF& r = selectionDrawRect;
    
    // 优化4：使用setRect避免临时对象构造
    // 绘制手柄（较小）
    drawHandles[0].setRect(r.left() - drawHalf, r.top() - drawHalf, drawSize, drawSize);
    drawHandles[1].setRect(r.right() - drawHalf, r.top() - drawHalf, drawSize, drawSize);
    drawHandles[2].setRect(r.left() - drawHalf, r.bottom() - drawHalf, drawSize, drawSize);
    drawHandles[3].setRect(r.right() - drawHalf, r.bottom() - drawHalf, drawSize, drawSize);
    drawHandles[4].setRect(r.left() - drawHalf, r.center().y() - drawHalf, drawSize, drawSize);
    drawHandles[5].setRect(r.right() - drawHalf, r.center().y() - drawHalf, drawSize, drawSize);
    drawHandles[6].setRect(r.center().x() - drawHalf, r.top() - drawHalf, drawSize, drawSize);
    drawHandles[7].setRect(r.center().x() - drawHalf, r.bottom() - drawHalf, drawSize, drawSize);
    
    // 命中手柄（较大）
    hitHandles[0].setRect(r.left() - hitHalf, r.top() - hitHalf, hitSize, hitSize);
    hitHandles[1].setRect(r.right() - hitHalf, r.top() - hitHalf, hitSize, hitSize);
    hitHandles[2].setRect(r.left() - hitHalf, r.bottom() - hitHalf, hitSize, hitSize);
    hitHandles[3].setRect(r.right() - hitHalf, r.bottom() - hitHalf, hitSize, hitSize);
    hitHandles[4].setRect(r.left() - hitHalf, r.center().y() - hitHalf, hitSize, hitSize);
    hitHandles[5].setRect(r.right() - hitHalf, r.center().y() - hitHalf, hitSize, hitSize);
    hitHandles[6].setRect(r.center().x() - hitHalf, r.top() - hitHalf, hitSize, hitSize);
    hitHandles[7].setRect(r.center().x() - hitHalf, r.bottom() - hitHalf, hitSize, hitSize);
}

QPointF CropOverlay::mapToImage(const QPointF& widgetPos) const {
    // 优化6：避免临时对象构造
    qreal x = (widgetPos.x() - imageDrawRect.left()) / scale;
    qreal y = (widgetPos.y() - imageDrawRect.top()) / scale;
    x = std::clamp(x, 0.0, imageRect.width());
    y = std::clamp(y, 0.0, imageRect.height());
    return {x, y};
}

//------------------------------------------------------------------------------
// 比例辅助函数
//------------------------------------------------------------------------------
void CropOverlay::applyAspectRatio() {
    if (!lockAspectRatio || !hasSelection()) return;
    QRectF newRect = selectionRect;
    QSizeF sz = newRect.size();
    QPointF center = newRect.center();
    if (sz.width() / sz.height() > cachedRatio) {
        sz.setWidth(sz.height() * cachedRatio);
    } else {
        sz.setHeight(sz.width() / cachedRatio);
    }
    newRect.setSize(sz);
    newRect.moveCenter(center);
    if (newRect.left() < 0) newRect.moveLeft(0);
    if (newRect.top() < 0) newRect.moveTop(0);
    if (newRect.right() > imageRect.right()) newRect.moveRight(imageRect.right());
    if (newRect.bottom() > imageRect.bottom()) newRect.moveBottom(imageRect.bottom());
    
    if (!fuzzyCompareRect(selectionRect, newRect)) {
        selectionRect = newRect;
        updateSelectionDrawRect();
        emit selectionChanged(selectionRect.toRect());
    }
}

QPointF CropOverlay::adjustPointForAspectRatio(const QPointF& anchor, const QPointF& point, qreal ratio) const {
    QPointF delta = point - anchor;
    qreal absDx = qAbs(delta.x());
    qreal absDy = qAbs(delta.y());
    if (absDx == 0 && absDy == 0) return point;
    int signX = delta.x() >= 0 ? 1 : -1;
    int signY = delta.y() >= 0 ? 1 : -1;
    if (absDx / absDy > ratio) {
        absDx = absDy * ratio;
    } else {
        absDy = absDx / ratio;
    }
    QPointF newDelta(absDx * signX, absDy * signY);
    return anchor + newDelta;
}

//------------------------------------------------------------------------------
// 交互逻辑 (重点修改 resizeSelection)
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
        QSizeF sz = newRect.size();

        // 决定以宽度还是高度为基准
        bool widthBased = true;
        switch (cursorAction) {
            case CursorAction::DragLeft:
            case CursorAction::DragRight:
                widthBased = true;
                break;
            case CursorAction::DragTop:
            case CursorAction::DragBottom:
                widthBased = false;
                break;
            default:
                widthBased = (qAbs(delta.x()) >= qAbs(delta.y()));
                break;
        }

        if (widthBased) {
            sz.setHeight(sz.width() / cachedRatio);
        } else {
            sz.setWidth(sz.height() * cachedRatio);
        }
        newRect.setSize(sz);

        // 保持固定点
        if (cursorAction == CursorAction::DragTopLeft) {
            newRect.moveBottomRight(resizeAnchor);
        } else if (cursorAction == CursorAction::DragTopRight) {
            newRect.moveBottomLeft(resizeAnchor);
        } else if (cursorAction == CursorAction::DragBottomLeft) {
            newRect.moveTopRight(resizeAnchor);
        } else if (cursorAction == CursorAction::DragBottomRight) {
            newRect.moveTopLeft(resizeAnchor);
        } else if (cursorAction == CursorAction::DragLeft) {
            newRect.moveRight(resizeAnchor.x());
        } else if (cursorAction == CursorAction::DragRight) {
            newRect.moveLeft(resizeAnchor.x());
        } else if (cursorAction == CursorAction::DragTop) {
            newRect.moveBottom(resizeAnchor.y());
        } else if (cursorAction == CursorAction::DragBottom) {
            newRect.moveTop(resizeAnchor.y());
        }
    }

    // 3. 边界约束（保持比例）
    QRectF clippedRect;
    // 优化7：仅在需要时调用 normalized()
    if (newRect.width() < 0 || newRect.height() < 0) {
        clippedRect = newRect.normalized().intersected(imageRect);
    } else {
        clippedRect = newRect.intersected(imageRect);
    }
    
    if (lockAspectRatio && (clippedRect != newRect)) {
        // 如果被裁剪，需要重新调整矩形，使其既符合比例又不超出边界
        QRectF boundedRect = clippedRect;
        
        // 先确定被裁剪的边，然后调整矩形使其贴合边界且比例正确
        bool leftClipped = (newRect.left() < imageRect.left());
        bool rightClipped = (newRect.right() > imageRect.right());
        bool topClipped = (newRect.top() < imageRect.top());
        bool bottomClipped = (newRect.bottom() > imageRect.bottom());
        
        if (leftClipped || rightClipped) {
            // 水平方向被限制，以宽度为基准调整高度
            qreal newWidth = boundedRect.width();
            qreal newHeight = newWidth / cachedRatio;
            boundedRect.setHeight(newHeight);
            
            // 根据拖拽方向调整垂直位置，确保矩形不超出垂直边界
            if (topClipped && !bottomClipped) {
                boundedRect.setTop(imageRect.top());
            } else if (bottomClipped && !topClipped) {
                boundedRect.setBottom(imageRect.bottom());
            } else if (topClipped && bottomClipped) {
                // 矩形过高，同时超出上下边界，此时应该缩小高度
                newHeight = imageRect.height();
                newWidth = newHeight * cachedRatio;
                boundedRect.setSize(QSizeF(newWidth, newHeight));
                boundedRect.moveTop(imageRect.top());
            } else {
                // 垂直方向未超限，尝试保持中心点
                qreal centerY = std::clamp(boundedRect.center().y(), imageRect.top(), imageRect.bottom());
                boundedRect.moveCenter(QPointF(boundedRect.center().x(), centerY));
            }
        } else if (topClipped || bottomClipped) {
            // 垂直方向被限制，以高度为基准调整宽度
            qreal newHeight = boundedRect.height();
            qreal newWidth = newHeight * cachedRatio;
            boundedRect.setWidth(newWidth);
            
            if (leftClipped && !rightClipped) {
                boundedRect.setLeft(imageRect.left());
            } else if (rightClipped && !leftClipped) {
                boundedRect.setRight(imageRect.right());
            } else if (leftClipped && rightClipped) {
                // 矩形过宽，同时超出左右边界
                newWidth = imageRect.width();
                newHeight = newWidth / cachedRatio;
                boundedRect.setSize(QSizeF(newWidth, newHeight));
                boundedRect.moveLeft(imageRect.left());
            } else {
                qreal centerX = std::clamp(boundedRect.center().x(), imageRect.left(), imageRect.right());
                boundedRect.moveCenter(QPointF(centerX, boundedRect.center().y()));
            }
        } else {
            // 角拖拽时可能同时超限，这种情况复杂，简单处理：直接按比例缩放并居中
            QSizeF sz = boundedRect.size();
            if (sz.width() / sz.height() > cachedRatio) {
                sz.setHeight(sz.width() / cachedRatio);
            } else {
                sz.setWidth(sz.height() * cachedRatio);
            }
            boundedRect.setSize(sz);
            boundedRect.moveCenter(clippedRect.center());
            // 最后确保在边界内
            boundedRect = boundedRect.intersected(imageRect);
        }
        
        if (!fuzzyCompareRect(selectionRect, boundedRect)) {
            selectionRect = boundedRect;
            updateSelectionDrawRect();
            emit selectionChanged(selectionRect.toRect());
        }
    } else {
        if (!fuzzyCompareRect(selectionRect, clippedRect)) {
            selectionRect = clippedRect;
            updateSelectionDrawRect();
            emit selectionChanged(selectionRect.toRect());
        }
    }
}

CursorAction CropOverlay::hoverTarget(const QPointF& pos) const {
    // 使用命中手柄数组（较大）进行检测
    if (hitHandles[0].contains(pos)) return CursorAction::DragTopLeft;
    if (hitHandles[1].contains(pos)) return CursorAction::DragTopRight;
    if (hitHandles[2].contains(pos)) return CursorAction::DragBottomLeft;
    if (hitHandles[3].contains(pos)) return CursorAction::DragBottomRight;
    if (hitHandles[4].contains(pos)) return CursorAction::DragLeft;
    if (hitHandles[5].contains(pos)) return CursorAction::DragRight;
    if (hitHandles[6].contains(pos)) return CursorAction::DragTop;
    if (hitHandles[7].contains(pos)) return CursorAction::DragBottom;
    if (selectionDrawRect.contains(pos)) return CursorAction::DragMove;
    return CursorAction::None;
}

void CropOverlay::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        moveStartPos = event->position();
        cursorAction = hoverTarget(moveStartPos);
        
        if (!hasSelection()) {
            cursorAction = CursorAction::SelectionStart;
            QPointF imgPos = mapToImage(moveStartPos);
            selectionRect.setTopLeft(imgPos);
            selectionRect.setBottomRight(imgPos);
            resizeAnchor = imgPos;
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
        
        // 优化8：分支顺序调整，将最常见路径放在前面
        if (cursorAction == CursorAction::DragMove) {
            QRectF newRect = selectionRect;
            newRect.translate(delta);
            if (newRect.left() < 0) newRect.moveLeft(0);
            if (newRect.right() > imageRect.right()) newRect.moveRight(imageRect.right());
            if (newRect.top() < 0) newRect.moveTop(0);
            if (newRect.bottom() > imageRect.bottom()) newRect.moveBottom(imageRect.bottom());
            
            // 优化3：仅当选区实际变化时才更新
            if (!fuzzyCompareRect(selectionRect, newRect)) {
                selectionRect = newRect;
                updateSelectionDrawRect();
                emit selectionChanged(selectionRect.toRect());
            }
        } 
        else if (cursorAction >= CursorAction::DragLeft && cursorAction <= CursorAction::DragBottomRight) {
            // resize group
            resizeSelection(delta);
        }
        else if (cursorAction == CursorAction::SelectionStart) {
            QPointF imgCurrent = mapToImage(currentPos);
            if (lockAspectRatio) {
                imgCurrent = adjustPointForAspectRatio(resizeAnchor, imgCurrent, cachedRatio);
            }
            QRectF newRect = QRectF(resizeAnchor, imgCurrent);
            if (newRect.width() < 0 || newRect.height() < 0) newRect = newRect.normalized();
            newRect = newRect.intersected(imageRect);
            
            if (!fuzzyCompareRect(selectionRect, newRect)) {
                selectionRect = newRect;
                updateSelectionDrawRect();
                emit selectionChanged(selectionRect.toRect());
            }
        }
        
        moveStartPos = currentPos;
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
    update(); // 确保手柄样式更新
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
        QRectF newRect = QRectF(selection).intersected(imageRect);
        if (!fuzzyCompareRect(selectionRect, newRect)) {
            selectionRect = newRect;
            updateSelectionDrawRect();
            emit selectionChanged(selectionRect.toRect());
        }
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

bool CropOverlay::fuzzyCompareRect(const QRectF& a, const QRectF& b) const {
    const qreal eps = 1e-6;
    return qFuzzyCompare(a.left(), b.left()) &&
           qFuzzyCompare(a.top(), b.top()) &&
           qFuzzyCompare(a.width(), b.width()) &&
           qFuzzyCompare(a.height(), b.height());
}