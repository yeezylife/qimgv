#ifndef STYLEDCOMBOBOX_H
#define STYLEDCOMBOBOX_H

#include <QComboBox>
#include <QPainter>
#include <QKeyEvent>
#include "settings.h"
#include "utils/imagelib.h"

class StyledComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit StyledComboBox(QWidget *parent = nullptr);
    void setIconPath(const QString& path);

protected:
    void paintEvent(QPaintEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    bool hiResPixmap = false;
    QPixmap downArrow;
    qreal dpr = 1.0;
    qreal pixmapDrawScale = 1.0;
};

#endif // STYLEDCOMBOBOX_H
