#include "thumbnailview.h"
#include <QApplication>
#include <QGuiApplication>

ThumbnailView::ThumbnailView(Qt::Orientation _orientation, QWidget *parent)
    : QGraphicsView(parent)
{
    setAccessibleName("thumbnailView");
    this->setMouseTracking(true);
    this->setAcceptDrops(false);
    this->setScene(&scene);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setAttribute(Qt::WA_TranslucentBackground, false);
    this->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    this->setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);

    setOrientation(_orientation);

    lastTouchpadScroll.start();

    connect(&loadTimer, &QTimer::timeout, this, &ThumbnailView::loadVisibleThumbnails);
    loadTimer.setInterval(static_cast<int>(LOAD_DELAY));
    loadTimer.setSingleShot(true);

    qreal screenMaxRefreshRate = 60.0;
    for(auto screen : qApp->screens()) {
        if(screen->refreshRate() > screenMaxRefreshRate)
            screenMaxRefreshRate = screen->refreshRate();
    }
    scrollRefreshRate = qRound(1000.0 / screenMaxRefreshRate);

    createScrollTimeLine();

    horizontalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    horizontalScrollBar()->installEventFilter(this);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailView::loadVisibleThumbnails);

    verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
    verticalScrollBar()->installEventFilter(this);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &ThumbnailView::loadVisibleThumbnails);

    if(qApp->platformName() == "wayland")
        wayland = true;
}

Qt::Orientation ThumbnailView::orientation() {
    return mOrientation;
}

void ThumbnailView::setOrientation(Qt::Orientation _orientation) {
    mOrientation = _orientation;
    if(mOrientation == Qt::Horizontal) {
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollBar = this->horizontalScrollBar();
        centerOn = [this](int value) {
            QGraphicsView::centerOn(static_cast<qreal>(value) + 1.0, viewportCenter.y());
        };
    } else {
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollBar = this->verticalScrollBar();
        centerOn = [this](int value) {
            QGraphicsView::centerOn(viewportCenter.x(), static_cast<qreal>(value) + 1.0);
        };
    }
}

void ThumbnailView::hideEvent(QHideEvent *event) {
    QGraphicsView::hideEvent(event);
    rangeSelection = false;
}

void ThumbnailView::createScrollTimeLine() {
    if(scrollTimeLine) {
        scrollTimeLine->stop();
        scrollTimeLine->deleteLater();
    }
    scrollTimeLine = new QTimeLine(SCROLL_DURATION, this);
    scrollTimeLine->setEasingCurve(QEasingCurve::OutSine);
    scrollTimeLine->setUpdateInterval(scrollRefreshRate);

    connect(scrollTimeLine, &QTimeLine::frameChanged, [this](int value) {
        scrollFrameTimer.start();
        this->centerOn(value);
        qApp->processEvents();
        lastScrollFrameTime = static_cast<int>(scrollFrameTimer.elapsed());
        if(scrollTimeLine->state() == QTimeLine::Running && lastScrollFrameTime > scrollRefreshRate) {
            scrollTimeLine->setPaused(true);
            int newTime = qMin(scrollTimeLine->duration(), scrollTimeLine->currentTime() + lastScrollFrameTime);
            scrollTimeLine->setCurrentTime(newTime);
            scrollTimeLine->setPaused(false);
        }
    });

    connect(scrollTimeLine, &QTimeLine::finished, [this]() {
        blockThumbnailLoading = false;
        loadVisibleThumbnails();
    });
}

bool ThumbnailView::eventFilter(QObject *o, QEvent *ev) {
    if (o == horizontalScrollBar() || o == verticalScrollBar()) {
        if(ev->type() == QEvent::Wheel) {
            this->wheelEvent(dynamic_cast<QWheelEvent*>(ev));
            return true;
        } else if(ev->type() == QEvent::Paint && mDrawScrollbarIndicator) {
            QPainter p(scrollBar);
            p.setOpacity(0.3);
            p.fillRect(indicator, QBrush(Qt::gray));
            p.setOpacity(1.0);
            return false;
        }
    }
    return QGraphicsView::eventFilter(o, ev);
}

void ThumbnailView::setDirectoryPath(QString path) {
    Q_UNUSED(path)
}

void ThumbnailView::select(QList<int> indices) {
    for(auto i : mSelection) {
        if(checkRange(i)) thumbnails.at(i)->setHighlighted(false);
    }
    
    QList<int> validIndices;
    for(int idx : indices) {
        if(checkRange(idx)) {
            thumbnails.at(idx)->setHighlighted(true);
            validIndices.append(idx);
        }
    }
    mSelection = validIndices;
    updateScrollbarIndicator();
}

void ThumbnailView::select(int index) {
    if(!checkRange(index))
        index = 0;
    this->select(QList<int>() << index);
}

void ThumbnailView::deselect(int index) {
    if(!checkRange(index))
            return;
    if(mSelection.count() > 1) {
        mSelection.removeAll(index);
        thumbnails.at(index)->setHighlighted(false);
    }
}

void ThumbnailView::addSelectionRange(int indexTo) {
    if(rangeSelectionSnapshot.isEmpty() || selection().isEmpty())
        return;
    
    auto list = rangeSelectionSnapshot;
    int lastIdx = rangeSelectionSnapshot.last();
    if(indexTo > lastIdx) {
        for(int i = lastIdx + 1; i <= indexTo; i++) {
            list.removeAll(i);
            list << i;
        }
    } else {
        for(int i = lastIdx - 1; i >= indexTo; i--) {
            list.removeAll(i);
            list << i;
        }
    }
    select(list);
}

QList<int> ThumbnailView::selection() {
    return mSelection;
}

void ThumbnailView::clearSelection() {
    for(auto i : mSelection) {
        if(checkRange(i)) thumbnails.at(i)->setHighlighted(false);
    }
    mSelection.clear();
}

int ThumbnailView::lastSelected() {
    return mSelection.isEmpty() ? -1 : mSelection.last();
}

int ThumbnailView::itemCount() {
    return static_cast<int>(thumbnails.count());
}

void ThumbnailView::show() {
    QGraphicsView::show();
    focusOnSelection();
    loadVisibleThumbnails();
}

void ThumbnailView::showEvent(QShowEvent *event) {
    QGraphicsView::showEvent(event);
    qApp->processEvents();
    updateScrollbarIndicator();
    loadVisibleThumbnails();
}

void ThumbnailView::populate(int newCount) {
    qApp->processEvents();
    clearSelection();
    lastScrollDirection = SCROLL_FORWARDS;
    this->setUpdatesEnabled(false);

    if(newCount >= 0) {
        if(newCount == static_cast<int>(thumbnails.count())) {
            for (auto* thumb : thumbnails) {
                thumb->reset();
            }
        } else {
            removeAll();
            for(int i = 0; i < newCount; i++) {
                ThumbnailWidget *widget = createThumbnailWidget();
                widget->setThumbnailSize(mThumbnailSize);
                thumbnails.append(widget);
                addItemToLayout(widget, i);
            }
        }
    }
    updateLayout();
    fitSceneToContents();
    resetViewport();
    qApp->processEvents();
    this->setUpdatesEnabled(true);
    loadVisibleThumbnails();
}

void ThumbnailView::addItem() {
    insertItem(itemCount());
}

void ThumbnailView::insertItem(int index) {
    ThumbnailWidget *widget = createThumbnailWidget();
    thumbnails.insert(index, widget);
    addItemToLayout(widget, index);
    updateLayout();
    fitSceneToContents();

    auto newSelection = mSelection;
    for(int i=0; i < newSelection.count(); i++) {
        if(index <= newSelection[i])
            newSelection[i]++;
    }
    select(newSelection);
    updateScrollbarIndicator();
    loadVisibleThumbnails();
}

void ThumbnailView::removeItem(int index) {
    if(checkRange(index)) {
        auto newSelection = mSelection;
        clearSelection();
        removeItemFromLayout(index);
        delete thumbnails.takeAt(index);
        fitSceneToContents();
        newSelection.removeAll(index);
        for(int i=0; i < newSelection.count(); i++) {
            if(newSelection[i] >= index)
                newSelection[i]--;
        }
        if(newSelection.isEmpty() && itemCount() > 0)
            newSelection << ((index >= itemCount()) ? itemCount() - 1 : index);
        select(newSelection);
        updateScrollbarIndicator();
        loadVisibleThumbnails();
    }
}

void ThumbnailView::reloadItem(int index) {
    Q_UNUSED(index)
    // 缩略图功能已禁用，不再发出重载信号
}

void ThumbnailView::setDragHover(int index) {
    Q_UNUSED(index)
}

void ThumbnailView::setCropThumbnails(bool mode) {
    mCropThumbnails = mode;
}

void ThumbnailView::setDrawScrollbarIndicator(bool mode) {
    mDrawScrollbarIndicator = mode;
}

void ThumbnailView::setThumbnail(int pos, std::shared_ptr<Thumbnail> thumb) {
    Q_UNUSED(pos)
    Q_UNUSED(thumb)
    // 缩略图功能已禁用
}

void ThumbnailView::unloadAllThumbnails() {
    for(auto* thumb : thumbnails) {
        thumb->unsetThumbnail();
    }
}

void ThumbnailView::loadVisibleThumbnails() {
    // 彻底切断所有缩略图计算和请求
    loadTimer.stop();
}

void ThumbnailView::loadVisibleThumbnailsDelayed() {
    loadTimer.stop();
}

void ThumbnailView::resetViewport() {
    if(scrollTimeLine && scrollTimeLine->state() == QTimeLine::Running)
        scrollTimeLine->stop();
    if(scrollBar)
        scrollBar->setValue(0);
}

int ThumbnailView::thumbnailSize() {
    return mThumbnailSize;
}

bool ThumbnailView::atSceneStart() {
    if(mOrientation == Qt::Horizontal) {
        return (viewportTransform().dx() == 0.0);
    } else {
        return (viewportTransform().dy() == 0.0);
    }
}

bool ThumbnailView::atSceneEnd() {
    if(mOrientation == Qt::Horizontal) {
        return (viewportTransform().dx() <= viewport()->width() - sceneRect().width());
    } else {
        return (viewportTransform().dy() <= viewport()->height() - sceneRect().height());
    }
}

bool ThumbnailView::checkRange(int pos) {
    return pos >= 0 && pos < static_cast<int>(thumbnails.count());
}

void ThumbnailView::updateLayout() {}

void ThumbnailView::fitSceneToContents() {
    if(this->mOrientation == Qt::Vertical) {
        int height = qMax(static_cast<int>(scene.itemsBoundingRect().height()), this->height());
        scene.setSceneRect(0, 0, this->width(), height);
        QPointF center = mapToScene(viewport()->rect().center());
        QGraphicsView::centerOn(0, center.y() + 1.0);
    } else {
        int width = qMax(static_cast<int>(scene.itemsBoundingRect().width()), this->width());
        scene.setSceneRect(0, 0, width, this->height());
        QPointF center = mapToScene(viewport()->rect().center());
        QGraphicsView::centerOn(center.x() + 1.0, 0);
    }
}

void ThumbnailView::wheelEvent(QWheelEvent *event) {
    event->accept();
    int pixelDelta = event->pixelDelta().y();
    int angleDelta = event->angleDelta().y();
    bool isWheel = true;

    if(settings->trackpadDetection()) {
        if(wayland)
            isWheel = (event->phase() == Qt::NoScrollPhase);
        else
            isWheel = angleDelta && (std::abs(angleDelta) >= 120 && !(angleDelta % 60)) && lastTouchpadScroll.elapsed() > 250;
    }

    if(isWheel) {
        angleDelta = qRound(static_cast<qreal>(angleDelta) * settings->mouseScrollingSpeed());
        if(!settings->enableSmoothScroll()) {
            scrollByItem(pixelDelta ? pixelDelta : angleDelta);
        } else if(angleDelta) {
            scrollSmooth(angleDelta, WHEEL_SCROLL_MULTIPLIER, SCROLL_ACCELERATION, true);
        }
    } else {
        lastTouchpadScroll.restart();
        scrollPrecise(std::abs(angleDelta) > std::abs(pixelDelta) ? angleDelta : pixelDelta);
    }
}

void ThumbnailView::scrollPrecise(int delta) {
    lastScrollDirection = (delta < 0) ? SCROLL_FORWARDS : SCROLL_BACKWARDS;
    viewportCenter = mapToScene(viewport()->rect().center());
    if(scrollTimeLine->state() == QTimeLine::Running) {
        scrollTimeLine->stop();
        blockThumbnailLoading = false;
    }
    if( (delta > 0 && atSceneStart()) || (delta < 0 && atSceneEnd()) )
        return;

    if(mOrientation == Qt::Horizontal)
        centerOn(qRound(viewportCenter.x() - delta));
    else
        centerOn(qRound(viewportCenter.y() - delta));
}

void ThumbnailView::scrollByItem(int delta) {
    int minScroll = qMin(thumbnailSize() / 2, 100);
    QRectF visRect = mapToScene(viewport()->geometry()).boundingRect().adjusted(-minScroll,-minScroll,minScroll,minScroll);
    QList<QGraphicsItem *> visibleItems = scene.items(visRect, Qt::ContainsItemBoundingRect, Qt::AscendingOrder);
    
    if(thumbnails.isEmpty() || visibleItems.isEmpty())
        return;

    int target = 0;
    ThumbnailWidget* widget = nullptr;
    if(delta > 0) {
        widget = qgraphicsitem_cast<ThumbnailWidget*>(visibleItems.first());
        if(widget) target = static_cast<int>(thumbnails.indexOf(widget)) - 1;
    } else {
        widget = qgraphicsitem_cast<ThumbnailWidget*>(visibleItems.last());
        if(widget) target = static_cast<int>(thumbnails.indexOf(widget)) + 1;
    }
    if(widget) scrollToItem(target);
}

void ThumbnailView::scrollToItem(int index) {
    if(!checkRange(index)) return;
    ThumbnailWidget *item = thumbnails.at(index);
    QRectF sRect = mapToScene(viewport()->rect()).boundingRect();
    QRectF iRect = item->mapRectToScene(item->rect());
    
    if(!sRect.contains(iRect)) {
        int delta = 0;
        if(mOrientation == Qt::Vertical) {
            delta = qRound((iRect.top() >= sRect.top()) ? sRect.bottom() - iRect.bottom() : sRect.top() - iRect.top());
        } else {
            delta = qRound((iRect.left() >= sRect.left()) ? sRect.right() - iRect.right() : sRect.left() - iRect.left());
        }
        
        if(settings->enableSmoothScroll())
            scrollSmooth(delta);
        else
            scrollPrecise(delta);
    }
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration, bool additive) {
    lastScrollDirection = (delta < 0) ? SCROLL_FORWARDS : SCROLL_BACKWARDS;
    viewportCenter = mapToScene(viewport()->rect().center());
    if( (delta > 0 && atSceneStart()) || (delta < 0 && atSceneEnd()) ) return;

    int center = qRound(mOrientation == Qt::Horizontal ? viewportCenter.x() : viewportCenter.y());
    bool redirect = false, accelerate = false;
    int newEndFrame = center - qRound(static_cast<qreal>(delta) * multiplier);

    if( (newEndFrame < center && center < scrollTimeLine->endFrame()) ||
        (newEndFrame > center && center > scrollTimeLine->endFrame()) ) {
        redirect = true;
    }

    if(scrollTimeLine->state() != QTimeLine::NotRunning) {      
        if(scrollTimeLine->currentTime() < SCROLL_ACCELERATION_THRESHOLD)
            accelerate = true;
        if(scrollTimeLine->endFrame() == center)
            createScrollTimeLine();
        if(!redirect && additive)
            newEndFrame = scrollTimeLine->endFrame() - qRound(static_cast<qreal>(delta) * multiplier * acceleration);
    }

    scrollTimeLine->stop();
    scrollTimeLine->setDuration(accelerate ? qRound(static_cast<float>(SCROLL_DURATION) / SCROLL_ACCELERATION) : SCROLL_DURATION);
    blockThumbnailLoading = true;
    scrollTimeLine->setFrameRange(center, newEndFrame);
    scrollTimeLine->start();
}

void ThumbnailView::scrollSmooth(int delta, qreal multiplier, qreal acceleration) {
    scrollSmooth(delta, multiplier, acceleration, false);
}

void ThumbnailView::scrollSmooth(int delta) {
    scrollSmooth(delta, 1.0, 1.0, false);
}

void ThumbnailView::mousePressEvent(QMouseEvent *event) {
    mouseReleaseSelect = false;
    dragStartPos = QPoint(0,0);
    ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
    if(item) {
        int index = static_cast<int>(thumbnails.indexOf(item));
        if(event->button() == Qt::LeftButton) {
            if(event->modifiers() & Qt::ControlModifier) {
                if(!selection().contains(index)) select(selection() << index);
                else deselect(index);
            } else if(event->modifiers() & Qt::ShiftModifier) {
                addSelectionRange(index);
            } else if (selection().count() <= 1) {
                if(selectMode == ACTIVATE_BY_PRESS) {
                    emit itemActivated(index);
                    return;
                } else select(index);
            } else mouseReleaseSelect = true;
            dragStartPos = event->position().toPoint();
        } else if(event->button() == Qt::RightButton) {
            select(index);
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void ThumbnailView::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    if(event->buttons() != Qt::LeftButton || mSelection.isEmpty()) return;
    if(QLineF(dragStartPos, event->position()).length() >= 40) {
        auto *item = dynamic_cast<ThumbnailWidget*>(itemAt(dragStartPos));
        if(item && mSelection.contains(static_cast<int>(thumbnails.indexOf(item))))
            emit draggedOut();
    }
}

void ThumbnailView::mouseReleaseEvent(QMouseEvent *event) {
    QGraphicsView::mouseReleaseEvent(event);
    if(mouseReleaseSelect && QLineF(dragStartPos, event->position()).length() < 40) {
        ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
        if(item) select(static_cast<int>(thumbnails.indexOf(item)));
    }
}

void ThumbnailView::mouseDoubleClickEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        ThumbnailWidget *item = dynamic_cast<ThumbnailWidget*>(itemAt(event->position().toPoint()));
        if(item) {
            emit itemActivated(static_cast<int>(thumbnails.indexOf(item)));
            return;
        }
    }
    event->ignore();
}

void ThumbnailView::focusOutEvent(QFocusEvent *event) {
    QGraphicsView::focusOutEvent(event);
    rangeSelection = false;
}

void ThumbnailView::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Shift) rangeSelectionSnapshot = selection();
    if(event->modifiers() & Qt::ShiftModifier) rangeSelection = true;
    QGraphicsView::keyPressEvent(event);
}

void ThumbnailView::keyReleaseEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Shift) rangeSelection = false;
    QGraphicsView::keyReleaseEvent(event);
}

void ThumbnailView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    updateScrollbarIndicator();
}

void ThumbnailView::setSelectMode(ThumbnailSelectMode mode) {
    selectMode = mode;
}