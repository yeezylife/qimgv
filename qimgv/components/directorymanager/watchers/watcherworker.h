#pragma once

#include <QObject>
#include <atomic>

class WatcherWorker : public QObject
{
    Q_OBJECT
public:
    WatcherWorker() = default;
    virtual void run() = 0;

public Q_SLOTS:
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