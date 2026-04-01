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
    bool needImmediateStart = false;

    {
        QMutexLocker locker(&mutex);

        if (!running) {
            bufferedRequest = req;
            if (!buffered) {
                buffered = true;
                needImmediateStart = true;
            }
        } else {
            bufferedRequest = req;
            if (!buffered) {
                buffered = true;
            }
        }
    }

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
    bool hasNextTask = false;
    ScalerRequest nextReq;

    QImage resultImage;
    ScalerRequest resultReq;

    {
        QMutexLocker locker(&mutex);
        running = false;

        if (buffered) {
            hasNextTask = true;
            nextReq = bufferedRequest;
        } else {
            resultImage = std::move(scaled);
            resultReq = std::move(req);
            // 任务完成且无缓冲任务，释放当前任务的图像引用
            startedRequest = ScalerRequest();
        }
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