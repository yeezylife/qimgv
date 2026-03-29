#include "imageinfooverlay.h"
#include "ui_imageinfooverlay.h"

ImageInfoOverlay::ImageInfoOverlay(FloatingWidgetContainer *parent) :
    OverlayWidget(parent),
    ui(new Ui::ImageInfoOverlay)
{
    ui->setupUi(this);
    ui->closeButton->setIconPath(":res/icons/common/overlay/close-dim16.png");
    ui->headerIcon->setIconPath(":res/icons/common/overlay/info16.png");
    entryStub.setFixedSize(280, 48);
    entryStub.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    connect(ui->closeButton,  &IconButton::clicked, this, &ImageInfoOverlay::hide);
    this->setPosition(FloatingWidgetPosition::RIGHT);

    if(parent)
        setContainerSize(parent->size());
}

ImageInfoOverlay::~ImageInfoOverlay() {
    delete ui;
    entries.clear();
}

void ImageInfoOverlay::setExifInfo(const QHash<QString, QString>& info) {
    // existing widgets are owned by QWidget hierarchy; we keep pool to avoid frequent realloc
    for (EntryInfoItem *entry : std::as_const(entries)) {
        ui->entryLayout->removeWidget(entry);
        entry->hide();
    }

    qsizetype entryCount = entries.count();
    if(entryCount < info.count()) {
        for(qsizetype i = entryCount; i < info.count(); i++) {
            entries.append(new EntryInfoItem(this));
        }
        entryCount = entries.count();
    }

    QHash<QString, QString>::const_iterator i = info.constBegin();
    qsizetype entryIdx = 0;
    while(i != info.constEnd()) {
        EntryInfoItem *item = entries.at(entryIdx);
        item->setInfo(i.key(), i.value());
        ui->entryLayout->addWidget(item);
        item->show();
        ++i;
        ++entryIdx;
    }

    // Hiding/showing entryStub causes flicker,
    // so we just remove it from layout and clear the text.
    // It's still there but basically not visible
    if(info.count()) {
        ui->entryLayout->removeWidget(&entryStub);
        entryStub.setText("");
        entryStub.hide();
    } else {
        ui->entryLayout->addWidget(&entryStub);
        entryStub.setText("<no metadata found>");
        entryStub.show();
    }

    if(!isHidden() && entryCount != info.count()) {
        // ensure layout size is recalculated before reposition
        if (ui->entryLayout) {
            ui->entryLayout->invalidate();
            ui->entryLayout->activate();
        }
        adjustSize();
        recalculateGeometry();
    }
}

void ImageInfoOverlay::show() {
    OverlayWidget::show();
    adjustSize();
    recalculateGeometry();
}

void ImageInfoOverlay::wheelEvent(QWheelEvent *event) {
    event->accept();
}