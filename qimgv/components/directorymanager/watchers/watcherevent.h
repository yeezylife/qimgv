#pragma once

#include <QString>
#include <QtGlobal>

class WatcherEvent {
public:
    // 强类型包装，防止参数混淆
    struct Cookie { quint32 value; };
    struct TimerId { qint32 value; };

    enum Type {
        None,
        MovedFrom,
        MovedTo,
        Modify
    };

    // 构造函数（内联实现）
    WatcherEvent(const QString &name, qint32 timerId, Type type = None) noexcept
        : mName(name), mCookie(0), mTimerId(timerId), mType(type) {}

    WatcherEvent(const QString &name, Cookie cookie, TimerId timerId, Type type = None) noexcept
        : mName(name), mCookie(cookie.value), mTimerId(timerId.value), mType(type) {}

    ~WatcherEvent() noexcept = default;

    // Getter / Setter（内联实现）
    QString name() const noexcept { return mName; }
    void setName(const QString &name) noexcept { mName = name; }

    quint32 cookie() const noexcept { return mCookie; }
    void setCookie(quint32 cookie) noexcept { mCookie = cookie; }

    qint32 timerId() const noexcept { return mTimerId; }
    void setTimerId(qint32 timerId) noexcept { mTimerId = timerId; }

    Type type() const noexcept { return mType; }
    void setType(Type type) noexcept { mType = type; }

private:
    QString mName;
    quint32 mCookie = 0;
    qint32 mTimerId = 0;
    Type mType = None;
};