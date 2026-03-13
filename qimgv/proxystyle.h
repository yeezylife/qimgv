#pragma once

#include <QApplication>
#include <QProxyStyle>
#include <QPainter>

class ProxyStyle : public QProxyStyle {
public:
    ~ProxyStyle() override = default;
    
    void drawPrimitive(PrimitiveElement element, 
                      const QStyleOption *option, 
                      QPainter *painter, 
                      const QWidget *widget = nullptr) const override;
};