#include "linuxworker.h"

#define TAG "[LinuxWatcherWorker]"

LinuxWorker::LinuxWorker() :
    fd(-1)
{
}

void LinuxWorker::setDescriptor(int desc) {
    Q_UNUSED(desc);
}

void LinuxWorker::run() {
    emit started();
    // 空实现
    emit finished();
}

void LinuxWorker::handleErrorCode(int code) {
    Q_UNUSED(code);
}