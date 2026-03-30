#ifndef DIRECTORYWATCHER_P_H
#define DIRECTORYWATCHER_P_H

#include "directorywatcher.h"
#include "watcherworker.h"

#include <QString>
#include <QThread>
#include <QScopedPointer>

class DirectoryWatcherPrivate : public QObject {
    Q_OBJECT
    Q_DECLARE_PUBLIC(DirectoryWatcher)

public:
    explicit DirectoryWatcherPrivate(DirectoryWatcher* qq, WatcherWorker* worker);
    ~DirectoryWatcherPrivate() override;

    DirectoryWatcher* q_ptr;
    QString currentDirectory;
    QScopedPointer<WatcherWorker> worker;
    QScopedPointer<QThread> workerThread;

private Q_SLOTS:
    void startWorker();

private:
    Q_DISABLE_COPY_MOVE(DirectoryWatcherPrivate)
};

#endif // DIRECTORYWATCHER_P_H