#include "scaler.h"
#include <QMutexLocker>
#include <memory>

Scaler::Scaler(Cache *_cache, QObject *parent)
    : QObject(parent),
      buffered(false),
      running(false),
      cache(_cache)
{
    pool = new QThreadPool(this);
    pool->setMaxThreadCount(4);
    runnable = new ScalerRunnable();
    runnable->setAutoDelete(false);
    
    connect(runnable, &ScalerRunnable::started, this, &Scaler::onTaskStart, Qt::DirectConnection);
    connect(runnable, &ScalerRunnable::finished, this, &Scaler::onTaskFinish, Qt::DirectConnection);
    
    connect(this, &Scaler::acceptScalingResult, this, &Scaler::slotForwardScaledResult, Qt::QueuedConnection);
}

void Scaler::requestScaled(ScalerRequest req) {
    QString toReserve;
    QString toRelease;
    bool needStart = false;
    ScalerRequest reqToStart;

    {
        QMutexLocker locker(&mutex);

        if (!running) {
            if (!buffered) {
                bufferedRequest = req;
                buffered = true;
                toReserve = req.image()->fileName();
                needStart = true;
                reqToStart = req;
            } else {
                if (bufferedRequest.image() != req.image()) {
                    toRelease = bufferedRequest.image()->fileName();
                    toReserve = req.image()->fileName();
                    bufferedRequest = req;
                } else {
                    bufferedRequest = req;
                }
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

    if (!toReserve.isEmpty()) {
        cache->reserve(toReserve);
    }
    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    if (needStart) {
        startRequest(reqToStart);
    }
}

void Scaler::onTaskStart(ScalerRequest req) {
    QMutexLocker locker(&mutex);
    running = true;
    
    if (buffered && bufferedRequest == req) {
        buffered = false;
    }
    startedRequest = req;
}

void Scaler::onTaskFinish(QImage scaled, ScalerRequest req) {
    QString toRelease;
    bool hasBuffered = false;
    ScalerRequest nextReq;

    {
        QMutexLocker locker(&mutex);
        running = false;

        if (buffered && bufferedRequest.image() == req.image()) {
            // 图片将继续使用，不释放
        } else {
            toRelease = req.image()->fileName();
        }

        if (buffered) {
            hasBuffered = true;
            nextReq = bufferedRequest;
        } else {
            emit acceptScalingResult(scaled, req);
        }
    }

    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    if (hasBuffered) {
        startRequest(nextReq);
    }
}

void Scaler::slotForwardScaledResult(QImage image, ScalerRequest req) {
    QPixmap result = QPixmap::fromImage(image);
    emit scalingFinished(result, req);
}

void Scaler::startRequest(ScalerRequest req) {
    QMutexLocker locker(&mutex);
    running = true;
    runnable->setRequest(req);
    pool->start(runnable);
}