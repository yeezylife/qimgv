#pragma once

#include <QHash>
#include <QString>
#include <QList>
#include <QVersionNumber>

class Actions
{
public:
    Actions();
    static Actions *getInstance();
    const QHash<QString, QVersionNumber> &getMap() const { return mActions; }
    const QList<QString> &getList() const { return mActionList; }

private:
    void init();
    QHash<QString, QVersionNumber> mActions;
    QList<QString> mActionList; // 预缓存的 action 列表
};

extern Actions *appActions;
