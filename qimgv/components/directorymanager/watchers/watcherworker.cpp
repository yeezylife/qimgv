#include "watcherworker.h"
#include <QDebug>

WatcherWorker::WatcherWorker() = default;

void WatcherWorker::setRunning(bool running) noexcept {
    isRunning.store(running, std::memory_order_release);
}
