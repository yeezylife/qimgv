#pragma once

#include <QWidget>
#include <QStyleOption>
#include <QPainter>
#include <QMouseEvent>
#include <memory>
#include "settings.h"
#include "utils/imagelib.h"

enum IconColorMode {
    ICON_COLOR_CUSTOM,
    ICON_COLOR_THEME,
    ICON_COLOR_SOURCE
};

class IconWidget : public QWidget {
    Q_OBJECT // 建议加上，虽然当前没用到自定义信号，但有 slots
public:
    explicit IconWidget(QWidget *parent = nullptr);
    ~IconWidget() override; // 明确使用 override

    // 修复 performance-unnecessary-value-param
    void setIconPath(const QString &path);
    
    // 修复 bugprone-easily-swappable-parameters (通过重载或统一使用 QPoint)
    void setIconOffset(const QPoint &offset);
    void setIconOffset(int x, int y); 

    void setColorMode(IconColorMode mode);
    void setColor(QColor color);
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onSettingsChanged();

private:
    void loadIcon();
    void applyColor();

    QString iconPath;
    QColor color;
    IconColorMode colorMode = ICON_COLOR_THEME;
    bool hiResPixmap = false;
    QPoint iconOffset;
    std::unique_ptr<QPixmap> pixmap; // 使用智能指针管理内存
    qreal dpr = 1.0;
    qreal pixmapDrawScale = 1.0;
};