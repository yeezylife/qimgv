#include "croppanel.h"
#include "ui_croppanel.h"

// RAII helper for blocking signals
class SignalBlocker {
public:
    SignalBlocker(QObject* obj) : m_obj(obj), m_blocked(false) {
        if (m_obj) {
            m_blocked = m_obj->blockSignals(true);
        }
    }

    ~SignalBlocker() {
        if (m_obj) {
            m_obj->blockSignals(m_blocked);
        }
    }

private:
    QObject* m_obj;
    bool m_blocked;
};

CropPanel::CropPanel(CropOverlay *_overlay, QWidget *parent) :
    SidePanelWidget(parent),
    ui(new Ui::CropPanel),
    overlay(_overlay)
{
    ui->setupUi(this);
    setFocusPolicy(Qt::NoFocus);

    ui->ARcomboBox->setItemDelegate(new QStyledItemDelegate(ui->ARcomboBox));
    ui->ARcomboBox->view()->setTextElideMode(Qt::ElideNone);

    ui->headerIcon->setIconPath(":/res/icons/common/other/image-crop48.png");

    ui->ARcomboBox->setIconPath(":res/icons/common/other/dropDownArrow.png");

    hide();

    if(settings->defaultCropAction() == ACTION_CROP)
        setFocusCropBtn();
    else
        setFocusCropSaveBtn();

    connect(ui->cropButton, &PushButtonFocusInd::rightPressed, this, &CropPanel::setFocusCropBtn);
    connect(ui->cropSaveButton, &PushButtonFocusInd::rightPressed, this, &CropPanel::setFocusCropSaveBtn);

    connect(ui->cancelButton, SIGNAL(clicked()), this, SIGNAL(cancel()));
    connect(ui->cropButton, SIGNAL(clicked()), this, SLOT(doCrop()));
    connect(ui->cropSaveButton, SIGNAL(clicked()), this, SLOT(doCropSave()));
    connect(ui->width, SIGNAL(valueChanged(int)), this, SLOT(onSelectionChange()));
    connect(ui->height, SIGNAL(valueChanged(int)), this, SLOT(onSelectionChange()));
    connect(ui->posX, SIGNAL(valueChanged(int)), this, SLOT(onSelectionChange()));
    connect(ui->posY, SIGNAL(valueChanged(int)), this, SLOT(onSelectionChange()));
    connect(ui->ARX, SIGNAL(valueChanged(double)), this, SLOT(onAspectRatioChange()));
    connect(ui->ARY, SIGNAL(valueChanged(double)), this, SLOT(onAspectRatioChange()));
    connect(ui->ARcomboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onAspectRatioSelected()));

    connect(overlay, SIGNAL(selectionChanged(QRect)),
            this, SLOT(onSelectionOutsideChange(QRect)));
    connect(this, SIGNAL(selectionChanged(QRect)),
            overlay, SLOT(onSelectionOutsideChange(QRect)));
    connect(this, SIGNAL(aspectRatioChanged(QPointF)),
            overlay, SLOT(setAspectRatio(QPointF)));
    connect(overlay, SIGNAL(escPressed()), this, SIGNAL(cancel()));
    connect(overlay, SIGNAL(cropDefault()), this, SLOT(doCropDefaultAction()));
    connect(overlay, SIGNAL(cropSave()), this, SLOT(doCropSave()));
    connect(this, SIGNAL(selectAll()), overlay, SLOT(selectAll()));
}

CropPanel::~CropPanel() {
    delete ui;
}

void CropPanel::setImageRealSize(QSize sz) {
    ui->width->setMaximum(sz.width());
    ui->height->setMaximum(sz.height());
    realSize = sz;

    ui->ARcomboBox->setCurrentIndex(0);

    onAspectRatioSelected();
}

void CropPanel::doCropDefaultAction() {
    if(settings->defaultCropAction() == ACTION_CROP)
        doCrop();
    else
        doCropSave();
}

void CropPanel::doCrop() {
    QRect target(ui->posX->value(), ui->posY->value(),
                 ui->width->value(), ui->height->value());

    if(target.width() > 0 && target.height() > 0 && target.size() != realSize)
        emit crop(target);
    else
        emit cancel();
}

void CropPanel::doCropSave() {
    QRect target(ui->posX->value(), ui->posY->value(),
                 ui->width->value(), ui->height->value());

    if(target.width() > 0 && target.height() > 0 && target.size() != realSize)
        emit cropAndSave(target);
    else
        emit cancel();
}

void CropPanel::onSelectionChange() {
    emit selectionChanged(QRect(ui->posX->value(),
                                ui->posY->value(),
                                ui->width->value(),
                                ui->height->value()));
}

void CropPanel::onAspectRatioChange() {
    ui->ARcomboBox->setCurrentIndex(1); // "Custom"

    if(ui->ARX->value() != 0.0 && ui->ARY->value() != 0.0)
        emit aspectRatioChanged(QPointF(ui->ARX->value(), ui->ARY->value()));
}

void CropPanel::onAspectRatioSelected() {
    QPointF newAR(1, 1);

    int index = ui->ARcomboBox->currentIndex();
    switch(index) {
    case 0:
    {
        overlay->setLockAspectRatio(false);
        if(realSize.height() != 0)
            newAR = QPointF(qreal(realSize.width()) / realSize.height(), 1.0);
        break;
    }
    case 1:
    {
        newAR = QPointF(ui->ARX->value(), ui->ARY->value());
        break;
    }
    case 2:
    {
        newAR = QPointF(qreal(realSize.width()) / realSize.height(), 1.0);
        break;
    }
    case 3:
    {
        QScreen* screen = nullptr;
#if QT_VERSION >= 0x050A00
        screen = QGuiApplication::screenAt(mapToGlobal(ui->ARcomboBox->geometry().topLeft()));
        if(!screen)
            screen = QGuiApplication::primaryScreen();
#else
        screen = QGuiApplication::primaryScreen();
#endif
        newAR = QPointF(qreal(screen->geometry().width()) / screen->geometry().height(), 1.0);
        break;
    }
    case 4:
        newAR = QPointF(1.0, 1.0);
        break;
    case 5:
        newAR = QPointF(4.0, 3.0);
        break;
    case 6:
        newAR = QPointF(16.0, 9.0);
        break;
    case 7:
        newAR = QPointF(16.0, 10.0);
        break;
    default:
        break;
    }

    ui->ARX->blockSignals(true);
    ui->ARY->blockSignals(true);
    ui->ARX->setValue(newAR.x());
    ui->ARY->setValue(newAR.y());
    ui->ARX->blockSignals(false);
    ui->ARY->blockSignals(false);

    if(index)
        overlay->setAspectRatio(newAR);
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
    SignalBlocker widthBlocker(ui->width);
    SignalBlocker heightBlocker(ui->height);
    SignalBlocker posXBlocker(ui->posX);
    SignalBlocker posYBlocker(ui->posY);

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
    QWidget::show();
    QTimer::singleShot(0,ui->width,SLOT(setFocus()));
}

void CropPanel::keyPressEvent(QKeyEvent *event) {
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if(event->modifiers() == Qt::ShiftModifier)
            doCropSave();
        else
            doCropDefaultAction();
    } else if(event->key() == Qt::Key_Escape) {
        emit cancel();
    } else if(event->matches(QKeySequence::SelectAll)) {
        emit selectAll();
    } else {
        event->ignore();
    }
}

void CropPanel::wheelEvent(QWheelEvent *event) {
    event->accept();
}