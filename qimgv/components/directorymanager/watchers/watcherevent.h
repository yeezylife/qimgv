#pragma once

#include <QString>
#include <QtGlobal>

class WatcherEvent {
public:
    // 方案 B：强类型定义，防止参数混淆
    struct Cookie { quint32 value; };
    struct TimerId { qint32 value; };

    enum Type {
        None,
        MovedFrom,
        MovedTo,
        Modify
    };

    // 构造函数现在接受强类型包装对象
    WatcherEvent(const QString &name, qint32 timerId, Type type = None) noexcept;
    WatcherEvent(const QString &name, Cookie cookie, TimerId timerId, Type type = None) noexcept;
    ~WatcherEvent() noexcept;

    QString name() const noexcept;
    void setName(const QString& name) noexcept;

    quint32 cookie() const noexcept;
    void setCookie(quint32 cookie) noexcept;

    qint32 timerId() const noexcept;
    void setTimerId(qint32 timerId) noexcept;

    Type type() const noexcept;
    void setType(Type type) noexcept;

private:
    QString mName;
    quint32 mCookie = 0;
    qint32 mTimerId = 0;
    Type mType = None;
};