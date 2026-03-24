#include "sidepanelwidget.h"

SidePanelWidget::SidePanelWidget(QWidget *parent) : QWidget(parent) {

}

// 添加实现，调用基类 QWidget 的 show 方法
void SidePanelWidget::show() {
    QWidget::show();
}