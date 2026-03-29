#include "scaler.h"
#include <QMutexLocker>

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

void Scaler::requestScaled(const ScalerRequest &req) {
    QString toReserve;
    QString toRelease;
    bool needImmediateStart = false;

    const auto& requestedImage = req.imageRef();
    const QString requestedFileName = requestedImage ? requestedImage->fileName() : QString();

    {
        QMutexLocker locker(&mutex);

        if (!running) {
            if (!buffered) {
                bufferedRequest = req;
                buffered = true;
                toReserve = requestedFileName;
                needImmediateStart = true;
            } else {
                const auto& existingImage = bufferedRequest.imageRef();
                if (existingImage != requestedImage) {
                    toRelease = existingImage ? existingImage->fileName() : QString();
                    toReserve = requestedFileName;
                }
                bufferedRequest = req;
            }
        } else {
            if (!buffered) {
                bufferedRequest = req;
                buffered = true;
                if (requestedImage != startedRequest.imageRef()) {
                    toReserve = requestedFileName;
                }
            } else {
                const auto& existingImage = bufferedRequest.imageRef();
                if (existingImage != requestedImage) {
                    if (existingImage != startedRequest.imageRef()) {
                        toRelease = existingImage ? existingImage->fileName() : QString();
                    }
                    if (requestedImage != startedRequest.imageRef()) {
                        toReserve = requestedFileName;
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

void Scaler::onTaskFinish(QImage scaled, ScalerRequest req) {
    QString toRelease;
    bool hasNextTask = false;
    ScalerRequest nextReq;

    QImage resultImage;
    ScalerRequest resultReq;

    const auto& finishedImage = req.imageRef();

    {
        QMutexLocker locker(&mutex);
        running = false;

        if (!(buffered && bufferedRequest.imageRef() == finishedImage)) {
            toRelease = finishedImage ? finishedImage->fileName() : QString();
        }

        if (buffered) {
            hasNextTask = true;
            nextReq = bufferedRequest;
        } else {
            resultImage = std::move(scaled);
            resultReq = std::move(req);
        }
    }

    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    if (hasNextTask) {
        startRequest(nextReq);
    }

    if (!resultImage.isNull()) {
        emit acceptScalingResult(std::move(resultImage), std::move(resultReq));
    }
}

void Scaler::slotForwardScaledResult(QImage image, ScalerRequest req) {
    QPixmap result = QPixmap::fromImage(std::move(image));
    emit scalingFinished(std::move(result), std::move(req));
}