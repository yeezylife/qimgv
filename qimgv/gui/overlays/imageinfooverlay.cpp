#include "imageinfooverlay.h"
#include "ui_imageinfooverlay.h"
#include <QStringList>

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
    // 信息未变且非空才直接返回，避免隐藏/重排全部条目造成的布局抖动；
    // 空信息必须走 entryStub 分支（m_lastInfo 初始为空，直接短路会导致
    // 首次打开无元数据图片时 "<no metadata found>" 提示永远不显示）
    if (!info.isEmpty() && info == m_lastInfo)
        return;
    m_lastInfo = info;

    // QHash 迭代顺序不确定，按键排序保证条目显示顺序稳定
    QStringList keys = info.keys();
    keys.sort();

    // existing widgets are owned by QWidget hierarchy; we keep pool to avoid frequent realloc
    for (EntryInfoItem *entry : std::as_const(entries)) {
        ui->entryLayout->removeWidget(entry);
        entry->hide();
    }

    qsizetype entryCount = entries.count();
    if(entryCount < keys.count()) {
        for(qsizetype i = entryCount; i < keys.count(); i++) {
            entries.append(new EntryInfoItem(this));
        }
        entryCount = entries.count();
    }

    qsizetype entryIdx = 0;
    for (const QString &key : keys) {
        EntryInfoItem *item = entries.at(entryIdx);
        item->setInfo(key, info.value(key));
        ui->entryLayout->addWidget(item);
        item->show();
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

    if(isVisible() && entryCount != info.count()) {
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