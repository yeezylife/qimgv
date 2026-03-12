#include "scaler.h"
#include <QMutexLocker>
#include <QDebug>

Scaler::Scaler(Cache *_cache, QObject *parent)
    : QObject(parent),
      buffered(false),
      running(false),
      cache(_cache)
{
    pool = new QThreadPool(this);
    pool->setMaxThreadCount(1); 
    
    // Qt6 会自动处理值传递的 QueuedConnection 优化
    connect(this, &Scaler::acceptScalingResult, this, &Scaler::slotForwardScaledResult, Qt::QueuedConnection);
}

Scaler::~Scaler() {
    pool->waitForDone();
}

void Scaler::requestScaled(const ScalerRequest &req) {
    QString toReserve;
    QString toRelease;
    bool needImmediateStart = false;

    {
        QMutexLocker locker(&mutex);

        if (!running) {
            if (!buffered) {
                bufferedRequest = req;
                buffered = true;
                toReserve = req.image()->fileName();
                needImmediateStart = true;
            } else {
                if (bufferedRequest.image() != req.image()) {
                    toRelease = bufferedRequest.image()->fileName();
                    toReserve = req.image()->fileName();
                }
                bufferedRequest = req;
            }
        } else {
            if (!buffered) {
                bufferedRequest = req;
                buffered = true;
                if (req.image() != startedRequest.image()) {
                    toReserve = req.image()->fileName();
                }
            } else {
                if (bufferedRequest.image() != req.image()) {
                    if (bufferedRequest.image() != startedRequest.image()) {
                        toRelease = bufferedRequest.image()->fileName();
                    }
                    if (req.image() != startedRequest.image()) {
                        toReserve = req.image()->fileName();
                    }
                    bufferedRequest = req;
                } else {
                    bufferedRequest = req;
                }
            }
        }
    }

    if (!toReserve.isEmpty()) cache->reserve(toReserve);
    if (!toRelease.isEmpty()) cache->release(toRelease);

    if (needImmediateStart) {
        startRequest(req);
    }
}

void Scaler::startRequest(const ScalerRequest& req) {
    auto *runnable = new ScalerRunnable(req);
    runnable->setAutoDelete(true);

    connect(runnable, &ScalerRunnable::started, this, &Scaler::onTaskStart, Qt::DirectConnection);
    connect(runnable, &ScalerRunnable::finished, this, &Scaler::onTaskFinish, Qt::DirectConnection);

    pool->start(runnable);
}

void Scaler::onTaskStart(const ScalerRequest &req) {
    QMutexLocker locker(&mutex);
    running = true;
    
    if (buffered && bufferedRequest == req) {
        buffered = false;
    }
    startedRequest = req;
}

// 修复点：参数移除 const &，改为值传递
void Scaler::onTaskFinish(QImage scaled, ScalerRequest req) {
    QString toRelease;
    bool hasNextTask = false;
    ScalerRequest nextReq;

    {
        QMutexLocker locker(&mutex);
        running = false;

        if (!(buffered && bufferedRequest.image() == req.image())) {
            toRelease = req.image()->fileName();
        }

        if (buffered) {
            hasNextTask = true;
            nextReq = bufferedRequest;
        } else {
            // 此时 scaled 是非 const 的左值，move 后将完美触发移动语义
            emit acceptScalingResult(std::move(scaled), std::move(req));
        }
    }

    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    if (hasNextTask) {
        startRequest(nextReq);
    }
}

// 修复点：参数移除 const &，改为值传递
void Scaler::slotForwardScaledResult(QImage image, ScalerRequest req) {
    // QPixmap::fromImage 在 Qt6 中针对右值有优化
    QPixmap result = QPixmap::fromImage(std::move(image));
    // 彻底消除警告，实现真正的资源转移
    emit scalingFinished(std::move(result), std::move(req));
}