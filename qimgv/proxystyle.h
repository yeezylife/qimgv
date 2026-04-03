#pragma once

#include <QProxyStyle>
#include <QPainter>

class ProxyStyle final : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;           // 继承所有构造函数
    ~ProxyStyle() override = default;

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override {
        if (element == PE_FrameFocusRect) {
            return;                           // 直接忽略焦点矩形
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};