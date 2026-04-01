// watcherworker.h
#pragma once

#include <QObject>
#include <atomic>

class WatcherWorker : public QObject
{
    Q_OBJECT
public:
    WatcherWorker() = default;
    virtual ~WatcherWorker() = default;

public Q_SLOTS:
    virtual void run() = 0;               // 改为槽，允许跨线程调用
    void setRunning(bool running) noexcept {
        isRunning.store(running, std::memory_order_release);
    }

Q_SIGNALS:
    void error(const QString &errorMessage);
    void started();
    void finished();

protected:
    std::atomic<bool> isRunning{false};
};