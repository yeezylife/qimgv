#pragma once

#include <QHash>
#include <QString>

class InputMap {
public:
    InputMap();
    static InputMap *getInstance();
    const QHash<quint32, QString> &keys();
    const QHash<QString, Qt::KeyboardModifier> &modifiers();
    static const QString& keyNameCtrl();
    static const QString& keyNameAlt();
    static const QString& keyNameShift();

private:
    void initKeyMap();
    void initModMap();
    QHash<quint32, QString> keyMap;
    QHash<QString, Qt::KeyboardModifier> modMap;
};

extern InputMap *inputMap;
