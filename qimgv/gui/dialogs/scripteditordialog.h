#pragma once

#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include "components/scriptmanager/scriptmanager.h"
#include "utils/script.h"

namespace Ui {
class ScriptEditorDialog;
}

class ScriptEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptEditorDialog(QWidget *parent = nullptr);
    // --- 修改这里：添加 const & ---
    explicit ScriptEditorDialog(const QString &name, const Script &script, QWidget *parent = nullptr);
    ~ScriptEditorDialog();
    QString scriptName();
    Script script();

private slots:
    void onNameChanged(const QString &name);

    void selectScriptPath();
private:
    void initializeDialog();
    void initializeEditMode(const QString &name, const Script &script);
    
    Ui::ScriptEditorDialog *ui;
    bool editMode;
    QString editTarget;
};