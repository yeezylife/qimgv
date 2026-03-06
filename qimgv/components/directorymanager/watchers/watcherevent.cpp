#include <QDebug>
#include "watcherevent.h"

WatcherEvent::WatcherEvent(const QString &name, int timerId, WatcherEvent::Type type) noexcept :
    mName(name),
    mTimerId(timerId),
    mType(type)
{
}

WatcherEvent::WatcherEvent(const QString& name, uint cookie, int timerId, Type type) noexcept :
    mName(name),
    mCookie(cookie),
    mTimerId(timerId),
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

int WatcherEvent::timerId() const noexcept {
    return mTimerId;
}

void WatcherEvent::setTimerId(int timerId) noexcept {
    mTimerId = timerId;
}

uint WatcherEvent::cookie() const noexcept {
    return mCookie;
}

void WatcherEvent::setCookie(uint cookie) noexcept {
    mCookie = cookie;
}
