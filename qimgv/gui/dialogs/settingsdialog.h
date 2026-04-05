#pragma once

#include <QDialog>
#include <QHash>
#include <QFileDialog>
#include <QColorDialog>
#include <QThreadPool>
#include <QTableWidget>
#include <QTextBrowser>
#include <QListWidget>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include "gui/customwidgets/colorselectorbutton.h"
#include "gui/dialogs/shortcutcreatordialog.h"
#include "gui/dialogs/scripteditordialog.h"
#include "settings.h"
#include "components/actionmanager/actionmanager.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
    void switchToPage(int number);

public slots:
    int exec();

private:
    void initializeLanguageMap();
    void setupColorSchemeConnections();
    void setupRadioGroups();
    void setupSidebar();
    void adjustSizeToContents();
    
    void readColorScheme();
    void setColorScheme(ColorScheme colors);
    void saveColorScheme();
    void readSettings();
    void readShortcuts();
    void readScripts();
    Ui::SettingsDialog *ui;

    void saveShortcuts();
    void addShortcutToTable(const QString &action, const QString &shortcut);
    void addScriptToList(const QString &name);
    void removeShortcutAt(int row);

    QMap<QString, QString> langs; // <"en_US", "English">
    QButtonGroup fitModeGrp, folderEndGrp, zoomIndGrp;
    
    // O(1) lookup caches for shortcuts and scripts
    QHash<QString, int> shortcutToRowMap; // shortcut string -> table row
    QHash<QString, int> scriptToRowMap;   // script name -> list row

private slots:
    void saveSettings();
    void saveSettingsAndClose();

    void addScript();
    void editScript();
    void editScript(QListWidgetItem *item);
    void editScript(const QString &name);
    void removeScript();

    void addShortcut();
    void editShortcut();
    void editShortcut(int row);
    void removeShortcut();
    void resetShortcuts();
    void selectMpvPath();
    void onBgOpacitySliderChanged(int value);
    void onExpandLimitSliderChanged(int value);
    void onZoomStepSliderChanged(int value);
    void onJPEGQualitySliderChanged(int value);
    void resetToDesktopTheme();    
    void onAutoResizeLimitSliderChanged(int value);
    void onMouseScrollingSpeedSliderChanged(int value);
    void resetZoomLevels();
signals:
    void settingsChanged();
};
