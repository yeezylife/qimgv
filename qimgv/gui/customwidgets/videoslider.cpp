#include "videoslider.h"

VideoSlider::VideoSlider(QWidget *parent) : QSlider(parent) {
}

void VideoSlider::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        event->accept();
        setValueAtCursor(event->position().toPoint());
    }
}

void VideoSlider::mouseMoveEvent(QMouseEvent *event) {
    if(event->buttons() & Qt::LeftButton) {
        event->accept();
        setValueAtCursor(event->position().toPoint());
    }
}

void VideoSlider::setValueAtCursor(QPoint pos) {
    if(orientation() == Qt::Vertical)
        setValue(minimum() + ((maximum() - minimum()) * (height() - pos.y())) / height() );
    else
        setValue(minimum() + ((maximum() - minimum()) * pos.x()) / width() );
    emit sliderMovedX(value());
}
