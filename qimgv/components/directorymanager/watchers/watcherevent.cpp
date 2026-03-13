#include <QDebug>
#include "watcherevent.h"

WatcherEvent::WatcherEvent(const QString &name, qint32 timerId, WatcherEvent::Type type) noexcept :
    mName(name),
    mCookie(0),
    mTimerId(timerId),
    mType(type)
{
}

// 方案 B 实现：通过 .value 访问内部数值
WatcherEvent::WatcherEvent(const QString &name,
                           Cookie cookie,
                           TimerId timerId,
                           Type type) noexcept
    : mName(name),
      mCookie(cookie.value),
      mTimerId(timerId.value),
      mType(type)
{
}

WatcherEvent::~WatcherEvent() noexcept = default;

QString WatcherEvent::name() const noexcept {
    return mName;
}

void WatcherEvent::setName(const QString &name) noexcept {
    mName = name;
}

WatcherEvent::Type WatcherEvent::type() const noexcept {
    return mType;
}

void WatcherEvent::setType(WatcherEvent::Type type) noexcept {
    mType = type;
}

qint32 WatcherEvent::timerId() const noexcept {
    return mTimerId;
}

void WatcherEvent::setTimerId(qint32 timerId) noexcept {
    mTimerId = timerId;
}

quint32 WatcherEvent::cookie() const noexcept {
    return mCookie;
}

void WatcherEvent::setCookie(quint32 cookie) noexcept {
    mCookie = cookie;
}