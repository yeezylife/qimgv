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
    
    connect(this, &Scaler::acceptScalingResult, this, &Scaler::slotForwardScaledResult, Qt::QueuedConnection);
}

Scaler::~Scaler() {
    pool->waitForDone();
}

// 修复：改为 const ScalerRequest &req
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

// 修复：改为 const ScalerRequest &req
void Scaler::onTaskStart(const ScalerRequest &req) {
    QMutexLocker locker(&mutex);
    running = true;
    
    if (buffered && bufferedRequest == req) {
        buffered = false;
    }
    startedRequest = req;
}

// 修复：改为 const QImage &scaled 和 const ScalerRequest &req
void Scaler::onTaskFinish(const QImage &scaled, const ScalerRequest &req) {
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
            // 优化：使用 std::move(scaled)，因为此后不再需要该临时结果
            emit acceptScalingResult(std::move(scaled), req);
        }
    }

    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    if (hasNextTask) {
        startRequest(nextReq);
    }
}

// 修复：改为 const QImage &image 和 const ScalerRequest &req
void Scaler::slotForwardScaledResult(const QImage &image, const ScalerRequest &req) {
    // 这里的 image 虽然是 const&，但 QPixmap::fromImage 会处理其内部数据
    QPixmap result = QPixmap::fromImage(image);
    // 优化：req 是最后一次使用，可以使用 std::move
    emit scalingFinished(std::move(result), req);
}