#include "cropoverlay.h"
#include <QtMath>

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
    if (lockAspectRatio && hasSelection()) {
        applyAspectRatio();
    }
}

void CropOverlay::setAspectRatio(const QPointF& ratio) {
    if (qFuzzyIsNull(ratio.x()) || qFuzzyIsNull(ratio.y())) return;
    aspectRatio = ratio;
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

    p.setPen(Qt::NoPen);
    p.fillRect(QRectF(imageDrawRect.left(), imageDrawRect.top(), imageDrawRect.width(), selectionDrawRect.top() - imageDrawRect.top()), brushInactiveTint);
    p.fillRect(QRectF(imageDrawRect.left(), selectionDrawRect.bottom(), imageDrawRect.width(), imageDrawRect.bottom() - selectionDrawRect.bottom()), brushInactiveTint);
    p.fillRect(QRectF(imageDrawRect.left(), selectionDrawRect.top(), selectionDrawRect.left() - imageDrawRect.left(), selectionDrawRect.height()), brushInactiveTint);
    p.fillRect(QRectF(selectionDrawRect.right(), selectionDrawRect.top(), imageDrawRect.right() - selectionDrawRect.right(), selectionDrawRect.height()), brushInactiveTint);

    p.setPen(selectionOutlinePen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(selectionDrawRect);

    if (cursorAction == CursorAction::None && selectionDrawRect.width() > 40) {
        p.setBrush(brushHandle);
        for (const auto& handle : handles) {
            p.drawRect(handle);
        }
    }
}

//------------------------------------------------------------------------------
// 几何计算
//------------------------------------------------------------------------------
void CropOverlay::updateSelectionDrawRect() {
    if (!hasSelection()) return;
    selectionDrawRect.setTopLeft(selectionRect.topLeft() * scale + imageDrawRect.topLeft());
    selectionDrawRect.setSize(selectionRect.size() * scale);
    updateHandlePositions();
}

void CropOverlay::updateHandlePositions() {
    const qreal hs = 6.0;
    const QRectF& r = selectionDrawRect;
    handles[0] = QRectF(r.left() - hs, r.top() - hs, hs*2, hs*2);
    handles[1] = QRectF(r.right() - hs, r.top() - hs, hs*2, hs*2);
    handles[2] = QRectF(r.left() - hs, r.bottom() - hs, hs*2, hs*2);
    handles[3] = QRectF(r.right() - hs, r.bottom() - hs, hs*2, hs*2);
    handles[4] = QRectF(r.left() - hs, r.center().y() - hs, hs*2, hs*2);
    handles[5] = QRectF(r.right() - hs, r.center().y() - hs, hs*2, hs*2);
    handles[6] = QRectF(r.center().x() - hs, r.top() - hs, hs*2, hs*2);
    handles[7] = QRectF(r.center().x() - hs, r.bottom() - hs, hs*2, hs*2);
}

QPointF CropOverlay::mapToImage(const QPointF& widgetPos) const {
    QPointF p = (widgetPos - imageDrawRect.topLeft()) / scale;
    return QPointF(std::clamp(p.x(), 0.0, imageRect.width()),
                   std::clamp(p.y(), 0.0, imageRect.height()));
}

//------------------------------------------------------------------------------
// 比例辅助函数
//------------------------------------------------------------------------------
void CropOverlay::applyAspectRatio() {
    if (!lockAspectRatio || !hasSelection()) return;
    qreal targetRatio = aspectRatio.x() / aspectRatio.y();
    QRectF newRect = selectionRect;
    QSizeF sz = newRect.size();
    QPointF center = newRect.center();
    if (sz.width() / sz.height() > targetRatio) {
        sz.setWidth(sz.height() * targetRatio);
    } else {
        sz.setHeight(sz.width() / targetRatio);
    }
    newRect.setSize(sz);
    newRect.moveCenter(center);
    if (newRect.left() < 0) newRect.moveLeft(0);
    if (newRect.top() < 0) newRect.moveTop(0);
    if (newRect.right() > imageRect.right()) newRect.moveRight(imageRect.right());
    if (newRect.bottom() > imageRect.bottom()) newRect.moveBottom(imageRect.bottom());
    selectionRect = newRect;
    updateSelectionDrawRect();
    update();
    emit selectionChanged(selectionRect.toRect());
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
        qreal targetRatio = aspectRatio.x() / aspectRatio.y();
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
            sz.setHeight(sz.width() / targetRatio);
        } else {
            sz.setWidth(sz.height() * targetRatio);
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
    // 先进行普通裁剪，但会破坏比例
    QRectF clippedRect = newRect.normalized().intersected(imageRect);
    
    if (lockAspectRatio && (clippedRect != newRect)) {
        // 如果被裁剪，需要重新调整矩形，使其既符合比例又不超出边界
        qreal targetRatio = aspectRatio.x() / aspectRatio.y();
        QRectF boundedRect = clippedRect;
        
        // 尝试保持矩形在边界内并符合比例
        // 先确定被裁剪的边，然后调整矩形使其贴合边界且比例正确
        bool leftClipped = (newRect.left() < imageRect.left());
        bool rightClipped = (newRect.right() > imageRect.right());
        bool topClipped = (newRect.top() < imageRect.top());
        bool bottomClipped = (newRect.bottom() > imageRect.bottom());
        
        if (leftClipped || rightClipped) {
            // 水平方向被限制，以宽度为基准调整高度
            qreal newWidth = boundedRect.width();
            qreal newHeight = newWidth / targetRatio;
            boundedRect.setHeight(newHeight);
            
            // 根据拖拽方向调整垂直位置，确保矩形不超出垂直边界
            if (topClipped && !bottomClipped) {
                boundedRect.setTop(imageRect.top());
            } else if (bottomClipped && !topClipped) {
                boundedRect.setBottom(imageRect.bottom());
            } else if (topClipped && bottomClipped) {
                // 矩形过高，同时超出上下边界，此时应该缩小高度
                newHeight = imageRect.height();
                newWidth = newHeight * targetRatio;
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
            qreal newWidth = newHeight * targetRatio;
            boundedRect.setWidth(newWidth);
            
            if (leftClipped && !rightClipped) {
                boundedRect.setLeft(imageRect.left());
            } else if (rightClipped && !leftClipped) {
                boundedRect.setRight(imageRect.right());
            } else if (leftClipped && rightClipped) {
                // 矩形过宽，同时超出左右边界
                newWidth = imageRect.width();
                newHeight = newWidth / targetRatio;
                boundedRect.setSize(QSizeF(newWidth, newHeight));
                boundedRect.moveLeft(imageRect.left());
            } else {
                qreal centerX = std::clamp(boundedRect.center().x(), imageRect.left(), imageRect.right());
                boundedRect.moveCenter(QPointF(centerX, boundedRect.center().y()));
            }
        } else {
            // 角拖拽时可能同时超限，这种情况复杂，简单处理：直接按比例缩放并居中
            // 这里可以尝试保持矩形在边界内，但需要精细处理，简单起见，使用之前版本的 applyAspectRatio 逻辑
            QSizeF sz = boundedRect.size();
            if (sz.width() / sz.height() > targetRatio) {
                sz.setHeight(sz.width() / targetRatio);
            } else {
                sz.setWidth(sz.height() * targetRatio);
            }
            boundedRect.setSize(sz);
            boundedRect.moveCenter(clippedRect.center());
            // 最后确保在边界内
            boundedRect = boundedRect.intersected(imageRect);
        }
        
        selectionRect = boundedRect;
    } else {
        selectionRect = clippedRect;
    }
    
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
        
        if (cursorAction == CursorAction::DragMove) {
            selectionRect.translate(delta);
            if (selectionRect.left() < 0) selectionRect.moveLeft(0);
            if (selectionRect.right() > imageRect.right()) selectionRect.moveRight(imageRect.right());
            if (selectionRect.top() < 0) selectionRect.moveTop(0);
            if (selectionRect.bottom() > imageRect.bottom()) selectionRect.moveBottom(imageRect.bottom());
            updateSelectionDrawRect();
        } 
        else if (cursorAction == CursorAction::SelectionStart) {
            QPointF imgCurrent = mapToImage(currentPos);
            if (lockAspectRatio) {
                qreal targetRatio = aspectRatio.x() / aspectRatio.y();
                imgCurrent = adjustPointForAspectRatio(resizeAnchor, imgCurrent, targetRatio);
            }
            selectionRect = QRectF(resizeAnchor, imgCurrent).normalized().intersected(imageRect);
            updateSelectionDrawRect();
        }
        else {
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