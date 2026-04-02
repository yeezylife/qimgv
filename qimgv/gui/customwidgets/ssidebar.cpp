#include "ssidebar.h"

SSideBar::SSideBar(QWidget *parent) : QWidget{parent} {
    layout = new QBoxLayout(QBoxLayout::TopToBottom);
    layout->setSpacing(0);
    layout->setContentsMargins(8,8,9,9);
    layout->addStretch();
    setLayout(layout);
    addEntry(":res/icons/common/settings/general32.png",    tr("General"));
    addEntry(":res/icons/common/settings/view32.png",       tr("View"));
    addEntry(":res/icons/common/settings/appearance32.png", tr("Theme"));
    addEntry(":res/icons/common/settings/shortcuts32.png",  tr("Controls"));
    addEntry(":res/icons/common/settings/terminal32.png",   tr("Scripts"));
    addEntry(":res/icons/common/settings/advanced32.png",   tr("Advanced"));
    addEntry(":res/icons/common/settings/about32.png",      tr("About"));
}

void SSideBar::addEntry(const QString &icon, const QString &name) {
    SSideBarItem *entry = new SSideBarItem(icon, name);
    layout->insertWidget(static_cast<int>(entries.count()), entry); // 修复警告：显式转换 qsizetype -> int
    entries.append(entry);
    if(entries.count() == 1)
        selectEntry(0);
}

void SSideBar::selectEntry(int idx) {
    if(idx >= 0 && idx < entries.count()) {
        if(mSelectedIndex >= 0 && mSelectedIndex < entries.count())
            entries[mSelectedIndex]->setHighlighted(false);
        entries[idx]->setHighlighted(true);
        mSelectedIndex = idx;
        emit entrySelected(idx);
    }
}

void SSideBar::mousePressEvent(QMouseEvent *event) {
    event->accept();
    if(!(event->buttons() & Qt::MouseButton::LeftButton))
        return;
    selectEntryAt(event->position().toPoint());

}

void SSideBar::mouseMoveEvent(QMouseEvent *event) {
    event->accept();
    if(!(event->buttons() & Qt::MouseButton::LeftButton))
        return;
    selectEntryAt(event->position().toPoint());
}

void SSideBar::selectEntryAt(QPoint pos) {
    int newSelection = -1;
    for(int i = 0; i < entries.count(); i++) {
        if(entries[i]->geometry().contains(pos)) {
            if(!entries[i]->highlighted())
                newSelection = i;
            break;
        }
    }
    selectEntry(newSelection);
}

void SSideBar::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PrimitiveElement::PE_Widget, &opt, &p, this);
}

// -------------------------------------------------------------------

SSideBarItem::SSideBarItem(const QString& icon, const QString& name, QWidget *parent)
    : QWidget(parent)
{
    if(palette().base().color().valueF() <= 0.45)
        iconWidget.setColor(QColor(184,184,185));
    else
        iconWidget.setColor(QColor(70,70,70));

    iconWidget.setIconPath(icon);
    textLabel.setText(name);

    layout = new QBoxLayout(QBoxLayout::LeftToRight);
    layout->setContentsMargins(6,4,6,4);
    layout->setSpacing(7);
    layout->addWidget(&iconWidget);
    layout->addWidget(&textLabel);
    layout->addStretch();
    setLayout(layout);
    setMouseTracking(true);
}

void SSideBarItem::setHighlighted(bool mode) {
    mHighlighted = mode;
    this->setProperty("checked", mHighlighted);
    style()->unpolish(this);
    style()->polish(this);
}

bool SSideBarItem::highlighted() const {
    return mHighlighted;
}

void SSideBarItem::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PrimitiveElement::PE_Widget, &opt, &p, this);
}