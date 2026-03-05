#pragma once

#include <QString>

class WatcherEvent {
public:
    enum Type {
        None,
        MovedFrom,
        MovedTo,
        Modify
    };

    WatcherEvent(const QString &name,int timerId, Type type = None) noexcept;
    WatcherEvent(const QString& name, uint cookie, int timerId, Type type = None) noexcept;
    ~WatcherEvent() noexcept;

    QString name() const noexcept;
    void setName(const QString& name) noexcept;

    uint cookie() const noexcept;
    void setCookie(uint cookie) noexcept;

    int timerId() const noexcept;
    void setTimerId(int timerId) noexcept;

    Type type() const noexcept;
    void setType(Type type) noexcept;

private:
    QString mName;
    uint mCookie;
    int mTimerId;
    Type mType;

};
