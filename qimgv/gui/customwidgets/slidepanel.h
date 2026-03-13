#pragma once

#include <QtGlobal>
#include <QTimeLine>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QBoxLayout>
#include <QTimer>
#include <QApplication> // 确保 qApp 可用
#include "floatingwidget.h"
#include "settings.h"
#include <memory>
#include <QDebug>

class SlidePanel : public FloatingWidget {
    Q_OBJECT
public:
    explicit SlidePanel(FloatingWidgetContainer *parent);
    ~SlidePanel();
    
    bool hasWidget();
    void setWidget(const std::shared_ptr<QWidget>& w);
    
    QRect triggerRect();
    bool layoutManaged();
    void setLayoutManaged(bool mode);

    void setPosition(PanelPosition);
    PanelPosition position();
    void hideAnimated();

public slots:
    void show();
    void hide();

private slots:
    void onAnimationFinish();
    void animationUpdate(int frame);

protected:
    QHBoxLayout mLayout;
    QGraphicsOpacityEffect *fadeEffect;
    int panelSize, slideAmount;
    std::shared_ptr<QWidget> mWidget;
    QRect mTriggerRect;
    
    void setAnimationRange(QPoint start, QPoint end);
    void saveStaticGeometry(QRect geometry);
    QRect staticGeometry();
    
    QTimer timer;
    QTimeLine timeline;
    QEasingCurve outCurve;
    const int ANIMATION_DURATION = 230;
    PanelPosition mPosition;

    // --- 核心修改：将计算逻辑改为 final 或非虚函数以供构造函数安全调用 ---
    void recalculateGeometryInternal(); 
    void updateTriggerRectInternal();

    // 保持虚函数接口，但内部调用私有逻辑
    virtual void recalculateGeometry();
    virtual void updateTriggerRect();

private:
    void setOrientation();
    Qt::Orientation mOrientation;
    QRect mStaticGeometry;
    qreal panelVisibleOpacity = 1.0;
    QPoint startPosition, endPosition;
    bool mLayoutManaged = false;
};