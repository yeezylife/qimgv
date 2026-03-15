#include "clickablelabel.h"

ClickableLabel::ClickableLabel() = default;

ClickableLabel::ClickableLabel(QWidget *parent) : QLabel(parent) {
}

ClickableLabel::ClickableLabel(const QString &text) {
    setText(text);
}

ClickableLabel::ClickableLabel(const QString &text, QWidget *parent) : QLabel(parent) {
    setText(text);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event)
    emit clicked();
}
