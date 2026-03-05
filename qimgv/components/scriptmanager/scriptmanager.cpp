#include "scriptmanager.h"

ScriptManager *scriptManager = nullptr;

ScriptManager::ScriptManager(QObject *parent)
    : QObject(parent)
{
}

ScriptManager::~ScriptManager() {
    if (scriptManager) {
        scriptManager->saveScripts();
        delete scriptManager;
        scriptManager = nullptr;
    }
}

ScriptManager *ScriptManager::getInstance() {
    if(!scriptManager) {
        scriptManager = new ScriptManager();
        scriptManager->readScripts();
    }
    return scriptManager;
}

void ScriptManager::runScript(const QString &scriptName, std::shared_ptr<Image> img) {
    if(scripts.contains(scriptName)) {
        Script script = scripts.value(scriptName);
        if(script.command.isEmpty())
            return;
        QProcess exec(this);

        auto arguments = splitCommandLine(script.command);
        processArguments(arguments, img);
        QString program = arguments.takeAt(0);

        if(script.blocking) {
            exec.start(program, arguments);
            if(!exec.waitForStarted()) {
                qDebug() << "Unable not run application/script." << program << " Make sure it is an executable.";
            }
            exec.waitForFinished(10000);
        } else {
            if(!exec.startDetached(program, arguments)) {
                QFileInfo fi(program);
                QString errorString;
                if(fi.isFile() && !fi.isExecutable())
                     errorString = "Error:  " + program + "  is not an executable.";
                else
                    errorString = "Error: unable run application/script. See README for working examples.";
                emit error(errorString);
                qWarning() << errorString;
            }
        }
    } else {
        qDebug() << "[ScriptManager] File " << scriptName << " does not exist.";
    }
}

QString ScriptManager::runCommand(const QString& cmd) {
    QProcess exec;
    QStringList cmdSplit = splitCommandLineImpl(cmd);
    exec.start(cmdSplit.takeAt(0), cmdSplit);
    exec.waitForFinished(2000);
    return exec.readAllStandardOutput();
}

void ScriptManager::runCommandDetached(const QString& cmd) {
    QStringList cmdSplit = splitCommandLineImpl(cmd);
    QProcess::startDetached(cmdSplit.takeAt(0), cmdSplit);
}

// TODO: what if filename contains one of the tags?
void ScriptManager::processArguments(QStringList &cmd, std::shared_ptr<Image> img) {
    for (auto& i : cmd) {
        if(i.contains("%file%"))
            i.replace("%file%", img.get()->filePath());
#ifdef __WIN32
        // force "\" as a directory separator
        i.replace("/", "\\");
        i.replace("\\\\", "\\");
#endif
    }
}

// thanks stackoverflow
QStringList ScriptManager::splitCommandLine(const QString &cmdLine) {
    return splitCommandLineImpl(cmdLine);
}

QStringList ScriptManager::splitCommandLineImpl(const QString &cmdLine) {
    QStringList list;
    QString arg;
    bool escape = false;
    enum { Idle, Arg, QuotedArg } state = Idle;
    
    for (QChar c : cmdLine) {
        switch (state) {
        case Idle:
            if(!escape && c == '"')
                state = QuotedArg;
            else if (escape || !c.isSpace()) {
                arg += c;
                state = Arg;
            }
            break;
        case Arg:
            if(!escape && c == '"')
                state = QuotedArg;
            else if(escape || !c.isSpace())
                arg += c;
            else {
                list << arg;
                arg.clear();
                state = Idle;
            }
            break;
        case QuotedArg:
            if(!escape && c == '"')
                state = arg.isEmpty() ? Idle : Arg;
            else
                arg += c;
            break;
        }
        escape = false;
    }
    if(!arg.isEmpty())
        list << arg;
    return list;
}


bool ScriptManager::scriptExists(const QString& scriptName) const {
    return scripts.contains(scriptName);
}

void ScriptManager::readScripts() {
    settings->readScripts(scripts);
}

void ScriptManager::saveScripts() {
    settings->saveScripts(scripts);
}

// replaces if it already exists
void ScriptManager::addScript(const QString& scriptName, const Script& script) {
    if(scripts.contains(scriptName)) {
        qDebug() << "[ScriptManager] Replacing script" << scriptName;
        scripts.remove(scriptName);
    }
    scripts.insert(scriptName, script);
}

void ScriptManager::removeScript(const QString& scriptName) {
    scripts.remove(scriptName);
}

const QMap<QString, Script>& ScriptManager::allScripts() const {
    return scriptManager->scripts;
}

QList<QString> ScriptManager::scriptNames() const {
    return scriptManager->scripts.keys();
}

Script ScriptManager::getScript(const QString& scriptName) const {
    return scripts.value(scriptName);
}
