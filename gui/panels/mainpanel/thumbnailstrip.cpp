#include "thumbnailstrip.h"

ThumbnailStrip::ThumbnailStrip(QWidget *parent)
    : ThumbnailView(THUMBNAILVIEW_HORIZONTAL, parent),
      panelSize(100),
      current(-1),
      thumbnailSpacing(0)
{
    // Load delay. Move to base class?
    connect(&loadTimer, SIGNAL(timeout()), this, SLOT(loadVisibleThumbnails()));
    loadTimer.setSingleShot(true);
    this->setFocusPolicy(Qt::NoFocus);
    setupLayout();
}

//  no layout; manual item positioning
//  graphical issues otherwise
void ThumbnailStrip::setupLayout() {
    this->setAlignment(Qt::AlignLeft | Qt::AlignTop);
}

ThumbnailLabel* ThumbnailStrip::createThumbnailWidget() {
    ThumbnailLabel *widget = new ThumbnailLabel();
    widget->setDrawLabel(true);
    widget->setHightlightStyle(HIGHLIGHT_TOPBAR);
    return widget;
}


void ThumbnailStrip::addItemToLayout(ThumbnailLabel* widget, int pos) {
    if(!checkRange(pos))
        return;

    if(pos == current)
        current++;
    scene.addItem(widget);
    updateThumbnailPositions(pos, thumbnails.count() - 1);
}

void ThumbnailStrip::removeItemFromLayout(int pos) {
    if(checkRange(pos)) {
        if(pos == current)
            current = -1;
        ThumbnailLabel *thumb = thumbnails.takeAt(pos);
        scene.removeItem(thumb);
        // move items left
        ThumbnailLabel *tmp;
        for(int i = pos; i < thumbnails.count(); i++) {
            tmp = thumbnails.at(i);
            tmp->moveBy(-tmp->boundingRect().width(), 0);
        }
        fitSceneToContents();
    }
}

void ThumbnailStrip::updateThumbnailPositions() {
    updateThumbnailPositions(0, thumbnails.count() - 1);
}

void ThumbnailStrip::updateThumbnailPositions(int start, int end) {
    if(start > end || !checkRange(start) || !checkRange(end)) {
        return;
    }
    // assume all thumbnails are the same size
    int thumbWidth = thumbnails.at(start)->boundingRect().width() + thumbnailSpacing;
    ThumbnailLabel *tmp;
    for(int i = start; i <= end; i++) {
        tmp = thumbnails.at(i);
        tmp->setPos(i * thumbWidth, 0);
    }
}

void ThumbnailStrip::highlightThumbnail(int pos) {
    // this code fires twice on click. fix later
    // also wont highlight new label after removing file
    if(current != pos) {
        ensureThumbnailVisible(pos);
        // disable highlighting on previous thumbnail
        if(checkRange(current)) {
            thumbnails.at(current)->setHighlighted(false, false);
        }
        // highlight the new one
        if(checkRange(pos)) {
            thumbnails.at(pos)->setHighlighted(true, false);
            current = pos;
        }
    }
    loadVisibleThumbnails();
}

void ThumbnailStrip::ensureThumbnailVisible(int pos) {
    if(checkRange(pos))
        ensureVisible(thumbnails.at(pos)->sceneBoundingRect(),
                      thumbnailSize / 2,
                      0);
}

void ThumbnailStrip::loadVisibleThumbnailsDelayed() {
    loadTimer.stop();
    loadTimer.start(LOAD_DELAY);
}

// scene stuff??
void ThumbnailStrip::setThumbnailSize(int newSize) {
    if(newSize >= 20) {
        thumbnailSize = newSize;
        for(int i=0; i<thumbnails.count(); i++) {
            thumbnails.at(i)->setThumbnailSize(newSize);
        }
        //scene.invalidate(scene.sceneRect());
        updateThumbnailPositions(0, thumbnails.count() - 1);
        fitSceneToContents();
        ensureThumbnailVisible(current);
    }
}

//  reimplement is base
/*
void ThumbnailStrip::removeItemAt(int pos) {
    lock();
    if(checkRange(pos)) {
        if(pos == current)
            current = -1;
        ThumbnailLabel *thumb = thumbnails.takeAt(pos);
        scene.removeItem(thumb);
        // move items left
        ThumbnailLabel *tmp;
        for(int i = pos; i < thumbnails.count(); i++) {
            tmp = thumbnails.at(i);
            tmp->moveBy(-tmp->boundingRect().width(), 0);
            tmp->setLabelNum(i);
        }
        updateSceneRect();
    }
    unlock();
}
*/

// resizes thumbnailSize to fit new widget size
// TODO: find some way to make this trigger while hidden
void ThumbnailStrip::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event)
    QWidget::resizeEvent(event);
    if(event->oldSize().height() != height())
        updateThumbnailSize();
    if(event->oldSize().width() < width())
        loadVisibleThumbnailsDelayed();
}

// update size based on widget's size
// reposition thumbnails within scene if needed
void ThumbnailStrip::updateThumbnailSize() {
    int newSize = height() - 23;
    if( newSize % 2 )
        --newSize;
    if(newSize != thumbnailSize) {
        setThumbnailSize(newSize);
    }
}
