#ifndef ENTRYINFOITEM_H
#define ENTRYINFOITEM_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QStyleOption>
#include <QPainter>

class EntryInfoItem : public QWidget
{
    Q_OBJECT
public:
    explicit EntryInfoItem(QWidget *parent = nullptr);
    void setInfo(const QString& _name, const QString& _value);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QHBoxLayout layout;
    QLabel nameLabel, valueLabel;
};

#endif // ENTRYINFOITEM_H
