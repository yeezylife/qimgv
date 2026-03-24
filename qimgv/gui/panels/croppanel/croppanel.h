#pragma once

#include <memory>
#include <QWidget>
#include <QScreen>
#include <QStyleOption>
#include <QStyledItemDelegate>
#include <QAbstractItemView>
#include <QPainter>
#include <QTimer>
#include <QKeyEvent>
#include <QWheelEvent>

#include "gui/customwidgets/sidepanelwidget.h"
#include "gui/overlays/cropoverlay.h"

// 移除不必要的 include 提高编译速度

namespace Ui {
class CropPanel;
}

class CropPanel : public SidePanelWidget
{
    Q_OBJECT

public:
    explicit CropPanel(CropOverlay *_overlay, QWidget *parent = nullptr);
    ~CropPanel() override; // 明确标注 override

    void setImageRealSize(QSize size);

public slots:
    void onSelectionOutsideChange(QRect rect);
    void show() override;

signals:
    void crop(const QRect& rect);
    void cropAndSave(const QRect& rect);
    void cancel();
    void cropClicked();
    void selectionChanged(const QRect& rect);
    void selectAll();
    void aspectRatioChanged(const QPointF& ratio);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void doCrop();
    void doCropSave();
    void onSelectionChange();
    void onAspectRatioChange();   // 响应 SpinBox 手动输入
    void onAspectRatioSelected(); // 响应 ComboBox 下拉选择
    void setFocusCropBtn();
    void setFocusCropSaveBtn();
    void doCropDefaultAction();

private:
    std::unique_ptr<Ui::CropPanel> ui; // 使用智能指针管理 UI 生命周期
    CropOverlay *overlay;
    QSize realSize;
};