#pragma once

#include <QBoxLayout>
#include "gui/customwidgets/floatingwidgetcontainer.h"
#include "gui/viewers/viewerwidget.h"
#include "gui/panels/mainpanel/mainpanel.h"
#include "gui/panels/infobar/infobarproxy.h"

// TODO: use a template here?

class DocumentWidget : public FloatingWidgetContainer {
public:
    DocumentWidget(ViewerWidget *viewWidget, InfoBarProxy *infoBar, QWidget* parent = nullptr);
    ViewerWidget *viewWidget();
    void setFocus();
    void hideFloatingPanel();
    void hideFloatingPanel(bool animated);
    void setPanelEnabled(bool mode);
    bool panelEnabled();
    void setInteractionEnabled(bool mode);
    void allowPanelInit();

public slots:
    void onFullscreenModeChanged(bool mode);

private slots:
    void setPanelPinned(bool mode);
    bool panelPinned();
    void readSettings();

protected:
    void enterEvent(QEnterEvent *event);
    void leaveEvent(QEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    QBoxLayout *layout, *layoutRoot;
    ViewerWidget *mViewWidget = nullptr;
    InfoBarProxy *mInfoBar = nullptr;
    MainPanel *mainPanel = nullptr;
    bool avoidPanelFlag = false;
    bool mPanelEnabled = false;
    bool mPanelFullscreenOnly = false;
    bool mIsFullscreen = false;
    bool mPanelPinned = false;
    bool mInteractionEnabled = false;
    bool mAllowPanelInit = false;
};
