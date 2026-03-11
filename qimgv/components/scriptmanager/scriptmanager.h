#pragma once

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QProcess>
#include <memory>
#include "utils/script.h"
#include "sourcecontainers/image.h"
#include "settings.h"

class ScriptManager : public QObject {
    Q_OBJECT
public:
    static ScriptManager* getInstance();
    ~ScriptManager();
    void runScript(const QString &scriptName, const std::shared_ptr<Image> &img);
    static QString runCommand(const QString& cmd);
    static void runCommandDetached(const QString& cmd);
    bool scriptExists(const QString& scriptName) const;
    void readScripts();
    void saveScripts();
    void removeScript(const QString& scriptName);
    const QMap<QString, Script>& allScripts() const;
    const QList<QString> scriptNames() const;
    Script getScript(const QString& scriptName) const;
    void addScript(const QString& scriptName, const Script& script);
    static QStringList splitCommandLine(const QString& cmdLine);

signals:
    void error(const QString& message);

private:
    explicit ScriptManager(QObject *parent = nullptr);
    QMap<QString, Script> scripts; // <name, script>
    void processArguments(QStringList& cmd, const std::shared_ptr<Image> &img) const;
};

extern ScriptManager *scriptManager;
