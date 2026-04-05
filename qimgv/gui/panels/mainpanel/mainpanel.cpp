#include "mainpanel.h"

MainPanel::MainPanel(FloatingWidgetContainer *parent) : SlidePanel(parent) {
// buttons stuff
buttonsWidget.setAccessibleName("panelButtonsWidget");
openButton       = new ActionButton("open", ":res/icons/common/buttons/panel/open20.png", 30, this);
openButton->setAccessibleName("ButtonSmall");
openButton->setTriggerMode(TriggerMode::PressTrigger);
settingsButton   = new ActionButton("openSettings", ":res/icons/common/buttons/panel/settings20.png", 30, this);
settingsButton->setAccessibleName("ButtonSmall");
settingsButton->setTriggerMode(TriggerMode::PressTrigger);
exitButton       = new ActionButton("exit", ":res/icons/common/buttons/panel/close16.png", 30, this);
exitButton->setAccessibleName("ButtonSmall");
exitButton->setTriggerMode(TriggerMode::PressTrigger);
pinButton = new ActionButton("", ":res/icons/common/buttons/panel/pin-panel20.png", 30, this);
pinButton->setAccessibleName("ButtonSmall");
pinButton->setTriggerMode(TriggerMode::PressTrigger);
pinButton->setCheckable(true);
connect(pinButton, &ActionButton::toggled, this, &MainPanel::onPinClicked);
buttonsLayout.setDirection(QBoxLayout::BottomToTop);
buttonsLayout.setSpacing(0);
buttonsLayout.addWidget(settingsButton);
buttonsLayout.addWidget(openButton);
buttonsLayout.addStretch(0);
buttonsLayout.addWidget(pinButton);
buttonsLayout.addWidget(exitButton);
buttonsWidget.setLayout(&buttonsLayout);
layout()->addWidget(&buttonsWidget);
// 注意：不在构造函数中调用 readSettings()，避免虚函数调用问题
// readSettings();
//connect(settings, SIGNAL(settingsChanged()), this, SLOT(readSettings()));
}

MainPanel::~MainPanel() = default;

void MainPanel::onPinClicked() {
bool mode = !settings->panelPinned();
pinButton->setChecked(mode);
settings->setPanelPinned(mode);
emit pinned(mode);
}

void MainPanel::setPosition(PanelPosition p) {
SlidePanel::setPosition(p);
switch(p) {
case PANEL_TOP:
buttonsLayout.setDirection(QBoxLayout::BottomToTop);
layout()->setContentsMargins(0,0,0,1);
buttonsLayout.setContentsMargins(4,0,0,0);
break;
case PANEL_BOTTOM:
buttonsLayout.setDirection(QBoxLayout::BottomToTop);
layout()->setContentsMargins(0,3,0,0);
buttonsLayout.setContentsMargins(4,0,0,0);
break;
case PANEL_LEFT:
buttonsLayout.setDirection(QBoxLayout::LeftToRight);
layout()->setContentsMargins(0,0,1,0);
buttonsLayout.setContentsMargins(0,0,0,4);
break;
case PANEL_RIGHT:
buttonsLayout.setDirection(QBoxLayout::LeftToRight);
layout()->setContentsMargins(1,0,0,0);
buttonsLayout.setContentsMargins(0,0,0,4);
break;
}
recalculateGeometry();
}

void MainPanel::setExitButtonEnabled(bool mode) {
exitButton->setHidden(!mode);
}

// 非虚辅助方法，用于安全计算尺寸（可在构造期间调用）
QSize MainPanel::calculateSizeHint() const {
return QSize(0, 0);
}

// 注意：实现文件中不要加 override
QSize MainPanel::sizeHint() const {
return calculateSizeHint();
}

void MainPanel::readSettings() {
auto newPos = settings->panelPosition();
if(newPos == PANEL_TOP || newPos == PANEL_BOTTOM) {
this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
// 使用非虚辅助方法代替 sizeHint()，避免构造期间虚函数调用问题
int h = calculateSizeHint().height();
if(h)
setFixedHeight(h);
setFixedWidth(QWIDGETSIZE_MAX);
} else {
this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
// 使用非虚辅助方法代替 sizeHint()，避免构造期间虚函数调用问题
int w = calculateSizeHint().width();
if(w)
setFixedWidth(w);
setFixedHeight(QWIDGETSIZE_MAX);
}
setPosition(newPos);
pinButton->setChecked(settings->panelPinned());
}

// draw separator line at bottom or top
void MainPanel::paintEvent(QPaintEvent *event) {
// 修复：调用正确的父类 SlidePanel::paintEvent 而不是 QWidget::paintEvent
SlidePanel::paintEvent(event);
// borders
QPainter p(this);
p.setPen(settings->colorScheme().folderview_hc);
switch(mPosition) {
case PANEL_TOP:
p.drawLine(rect().bottomLeft(), rect().bottomRight());
break;
case PANEL_BOTTOM:
p.fillRect(rect().left(), rect().top(), width(), 3, settings->colorScheme().folderview);
p.drawLine(rect().topLeft(), rect().topRight());
break;
case PANEL_LEFT:
p.drawLine(rect().topRight(), rect().bottomRight());
break;
case PANEL_RIGHT:
p.drawLine(rect().topLeft(), rect().bottomLeft());
break;
}
}