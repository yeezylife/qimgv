#include "linuxwatcher_p.h"
#include "linuxworker.h"

#define TAG "[LinuxDirectoryWatcher]"

LinuxWatcherPrivate::LinuxWatcherPrivate(LinuxWatcher* qq) :
    DirectoryWatcherPrivate(qq, new LinuxWorker()),
    watcher(-1),
    watchObject(-1)
{
}

int LinuxWatcherPrivate::indexOfWatcherEvent(uint cookie) const {
    Q_UNUSED(cookie);
    return -1;
}

int LinuxWatcherPrivate::indexOfWatcherEvent(const QString& name) const {
    Q_UNUSED(name);
    return -1;
}

void LinuxWatcherPrivate::dispatchFilesystemEvent(LinuxFsEvent* e) {
    Q_UNUSED(e);
}

void LinuxWatcherPrivate::handleModifyEvent(const QString &name) {
    Q_UNUSED(name);
}

void LinuxWatcherPrivate::handleDeleteEvent(const QString &name) {
    Q_UNUSED(name);
}

void LinuxWatcherPrivate::handleCreateEvent(const QString &name) {
    Q_UNUSED(name);
}

void LinuxWatcherPrivate::handleMovedFromEvent(const QString &name, uint cookie) {
    Q_UNUSED(name);
    Q_UNUSED(cookie);
}

void LinuxWatcherPrivate::handleMovedToEvent(const QString &name, uint cookie) {
    Q_UNUSED(name);
    Q_UNUSED(cookie);
}

void LinuxWatcherPrivate::timerEvent(QTimerEvent *timerEvent) {
    Q_UNUSED(timerEvent);
}

LinuxWatcher::LinuxWatcher() : DirectoryWatcher(new LinuxWatcherPrivate(this)) {
}

LinuxWatcher::~LinuxWatcher() {
}

void LinuxWatcher::setWatchPath(const QString& path) {
    DirectoryWatcher::setWatchPath(path);
}