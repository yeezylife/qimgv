#include "croppanel.h"
#include "ui_croppanel.h"
#include <QSignalBlocker>
#include <QGuiApplication>

CropPanel::CropPanel(CropOverlay *_overlay, QWidget *parent) :
    SidePanelWidget(parent),
    ui(std::make_unique<Ui::CropPanel>()),
    overlay(_overlay)
{
    ui->setupUi(this);
    setFocusPolicy(Qt::NoFocus);

    // 下拉框美化
    ui->ARcomboBox->setItemDelegate(new QStyledItemDelegate(ui->ARcomboBox));
    ui->ARcomboBox->view()->setTextElideMode(Qt::ElideNone);
    ui->headerIcon->setIconPath(":/res/icons/common/other/image-crop48.png");
    ui->ARcomboBox->setIconPath(":res/icons/common/other/dropDownArrow.png");

    hide();

    // 初始化焦点状态
    if (settings->defaultCropAction() == ACTION_CROP)
        setFocusCropBtn();
    else
        setFocusCropSaveBtn();

    // --- 使用 Qt6 函数指针语法进行信号槽连接 ---
    
    // UI 内部交互
    connect(ui->cropButton, &PushButtonFocusInd::rightPressed, this, &CropPanel::setFocusCropBtn);
    connect(ui->cropSaveButton, &PushButtonFocusInd::rightPressed, this, &CropPanel::setFocusCropSaveBtn);

    connect(ui->cancelButton, &QPushButton::clicked, this, &CropPanel::cancel);
    connect(ui->cropButton, &QPushButton::clicked, this, &CropPanel::doCrop);
    connect(ui->cropSaveButton, &QPushButton::clicked, this, &CropPanel::doCropSave);

    // 数值变化同步
    auto syncSelection = &CropPanel::onSelectionChange;
    connect(ui->width, &QSpinBox::valueChanged, this, syncSelection);
    connect(ui->height, &QSpinBox::valueChanged, this, syncSelection);
    connect(ui->posX, &QSpinBox::valueChanged, this, syncSelection);
    connect(ui->posY, &QSpinBox::valueChanged, this, syncSelection);

    // 比例调节同步
    auto syncARChange = &CropPanel::onAspectRatioChange;
    connect(ui->ARX, &QDoubleSpinBox::valueChanged, this, syncARChange);
    connect(ui->ARY, &QDoubleSpinBox::valueChanged, this, syncARChange);
    connect(ui->ARcomboBox, &QComboBox::currentIndexChanged, this, &CropPanel::onAspectRatioSelected);

    // 与 Overlay 的双向通信
    connect(overlay, &CropOverlay::selectionChanged, this, &CropPanel::onSelectionOutsideChange);
    connect(this, &CropPanel::selectionChanged, overlay, &CropOverlay::onSelectionOutsideChange);
    connect(this, &CropPanel::aspectRatioChanged, overlay, &CropOverlay::setAspectRatio);
    
    connect(overlay, &CropOverlay::escPressed, this, &CropPanel::cancel);
    connect(overlay, &CropOverlay::cropDefault, this, &CropPanel::doCropDefaultAction);
    connect(overlay, &CropOverlay::cropSave, this, &CropPanel::doCropSave);
    connect(this, &CropPanel::selectAll, overlay, &CropOverlay::selectAll);
}

CropPanel::~CropPanel() = default; // std::unique_ptr 会自动处理释放

void CropPanel::setImageRealSize(QSize sz) {
    ui->width->setMaximum(sz.width());
    ui->height->setMaximum(sz.height());
    realSize = sz;

    // 重置为自由模式（Index 0），触发 onAspectRatioSelected 更新
    ui->ARcomboBox->setCurrentIndex(0);
    onAspectRatioSelected();
}

void CropPanel::doCropDefaultAction() {
    if (settings->defaultCropAction() == ACTION_CROP)
        doCrop();
    else
        doCropSave();
}

void CropPanel::doCrop() {
    QRect target(ui->posX->value(), ui->posY->value(), ui->width->value(), ui->height->value());
    if (target.isValid() && target.size() != realSize)
        emit crop(target);
    else
        emit cancel();
}

void CropPanel::doCropSave() {
    QRect target(ui->posX->value(), ui->posY->value(), ui->width->value(), ui->height->value());
    if (target.isValid() && target.size() != realSize)
        emit cropAndSave(target);
    else
        emit cancel();
}

void CropPanel::onSelectionChange() {
    emit selectionChanged(QRect(ui->posX->value(), ui->posY->value(),
                                ui->width->value(), ui->height->value()));
}

void CropPanel::onAspectRatioChange() {
    // 防止循环触发：手动修改数值时自动切换到 "Custom" 模式
    const QSignalBlocker blocker(ui->ARcomboBox);
    ui->ARcomboBox->setCurrentIndex(1); 

    if (ui->ARX->value() > 0.0 && ui->ARY->value() > 0.0)
        emit aspectRatioChanged(QPointF(ui->ARX->value(), ui->ARY->value()));
}

void CropPanel::onAspectRatioSelected() {
    QPointF newAR(1.0, 1.0);
    int index = ui->ARcomboBox->currentIndex();

    switch(index) {
        case 0: // Free Mode
            overlay->setLockAspectRatio(false);
            [[fallthrough]]; // 继续执行 case 2 的计算逻辑逻辑
        case 2: // Original Image
            if (realSize.height() > 0)
                newAR = QPointF(static_cast<qreal>(realSize.width()) / realSize.height(), 1.0);
            break;

        case 1: // Custom
            newAR = QPointF(ui->ARX->value(), ui->ARY->value());
            break;

        case 3: { // Screen
            QScreen* screen = QGuiApplication::screenAt(mapToGlobal(ui->ARcomboBox->geometry().topLeft()));
            if (!screen) screen = QGuiApplication::primaryScreen();
            if (screen) {
                QSize ss = screen->size();
                newAR = QPointF(static_cast<qreal>(ss.width()) / ss.height(), 1.0);
            }
            break;
        }
        case 4: newAR = QPointF(1.0, 1.0); break;
        case 5: newAR = QPointF(4.0, 3.0); break;
        case 6: newAR = QPointF(16.0, 9.0); break;
        case 7: newAR = QPointF(16.0, 10.0); break;
        default: return;
    }

    // 更新 UI 数值显示，屏蔽信号防止死循环
    {
        const QSignalBlocker blockerX(ui->ARX);
        const QSignalBlocker blockerY(ui->ARY);
        ui->ARX->setValue(newAR.x());
        ui->ARY->setValue(newAR.y());
    }

    if (index != 0) {
        overlay->setLockAspectRatio(true);
        overlay->setAspectRatio(newAR);
    }
}

void CropPanel::setFocusCropBtn() {
    settings->setDefaultCropAction(ACTION_CROP);
    ui->cropSaveButton->setHighlighted(false);
    ui->cropButton->setHighlighted(true);
}

void CropPanel::setFocusCropSaveBtn() {
    settings->setDefaultCropAction(ACTION_CROP_SAVE);
    ui->cropSaveButton->setHighlighted(true);
    ui->cropButton->setHighlighted(false);
}

void CropPanel::onSelectionOutsideChange(QRect rect) {
    // 批量阻塞信号
    const QSignalBlocker b1(ui->width);
    const QSignalBlocker b2(ui->height);
    const QSignalBlocker b3(ui->posX);
    const QSignalBlocker b4(ui->posY);

    ui->width->setValue(rect.width());
    ui->height->setValue(rect.height());
    ui->posX->setValue(rect.left());
    ui->posY->setValue(rect.top());
}

void CropPanel::paintEvent(QPaintEvent *) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void CropPanel::show() {
    SidePanelWidget::show();
    // 使用新的 Lambda 语法连接 Timer，更现代
    QTimer::singleShot(0, this, [this](){ ui->width->setFocus(); });
}

void CropPanel::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            (event->modifiers() & Qt::ShiftModifier) ? doCropSave() : doCropDefaultAction();
            break;
        case Qt::Key_Escape:
            emit cancel();
            break;
        default:
            if (event->matches(QKeySequence::SelectAll))
                emit selectAll();
            else
                event->ignore();
    }
}

void CropPanel::wheelEvent(QWheelEvent *event) {
    event->accept(); // 拦截滚轮，防止侧边栏滚动
}