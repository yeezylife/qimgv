#include "menuitem.h"

MenuItem::MenuItem(QWidget *parent)
    : QWidget(parent)
{
    mLayout.setContentsMargins(6,0,8,0);
    mLayout.setSpacing(2);

    setAccessibleName("MenuItem");
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    mTextLabel.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mShortcutLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mIconWidget.setMinimumSize(26, 26); // 5px padding from stylesheet

    mIconWidget.installEventFilter(this);

    spacer = new QSpacerItem(3, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    mIconWidget.setAttribute(Qt::WA_TransparentForMouseEvents, true);
    mIconWidget.setAccessibleName("MenuItemIcon");
    mTextLabel.setAccessibleName("MenuItemText");
    mShortcutLabel.setAccessibleName("MenuItemShortcutLabel");
    mLayout.addWidget(&mIconWidget);
    mLayout.addWidget(&mTextLabel);
    mLayout.addSpacerItem(spacer);
    mLayout.addWidget(&mShortcutLabel);
    mLayout.setStretch(1,1);

    setLayout(&mLayout);
}

MenuItem::~MenuItem() {
    delete spacer;
}

void MenuItem::setText(const QString &text) {
    mTextLabel.setText(text);
}

QString MenuItem::text() {
    return mTextLabel.text();
}

// 修复：改为 const QString &
void MenuItem::setShortcutText(const QString &text) {
    mShortcutLabel.setText(text);
    adjustSize();
}

QString MenuItem::shortcut() {
    return mShortcutLabel.text();
}

// 修复：使用 std::move 消除警告
// 这样 path 会直接移动到目标位置，避免了额外的引用计数增减
void MenuItem::setIconPath(QString path) {
    mIconWidget.setIconPath(std::move(path));
}

void MenuItem::setPassthroughClicks(bool mode) {
    passthroughClicks = mode;
}

void MenuItem::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void MenuItem::onPress() {
}

void MenuItem::onClick() {
}

void MenuItem::mousePressEvent(QMouseEvent *event) {
    onPress();
    QWidget::mousePressEvent(event);
    if(!passthroughClicks)
        event->accept();
}

void MenuItem::mouseReleaseEvent(QMouseEvent *event) {
    onClick();
    QWidget::mouseReleaseEvent(event);
    if(!passthroughClicks)
        event->accept();
}