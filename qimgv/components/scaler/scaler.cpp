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

    // ✅ queued 保持线程安全，但不会复制图像数据
    connect(this, &Scaler::acceptScalingResult,
            this, &Scaler::slotForwardScaledResult,
            Qt::QueuedConnection);
}

Scaler::~Scaler() {
    pool->waitForDone();
}

void Scaler::requestScaled(const ScalerRequest &req) {
    bool needImmediateStart = false;

    {
        QMutexLocker locker(&mutex);

        bufferedRequest = req;

        if (!running) {
            if (!buffered) {
                buffered = true;
                needImmediateStart = true;
            }
        } else {
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

    connect(runnable, &ScalerRunnable::started,
            this, &Scaler::onTaskStart,
            Qt::DirectConnection);

    // ✅ 关键：改为 shared pointer
    connect(runnable, &ScalerRunnable::finished,
            this, &Scaler::onTaskFinish,
            Qt::DirectConnection);

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

void Scaler::onTaskFinish(QSharedPointer<QImage> scaled, ScalerRequest req) {
    bool hasNextTask = false;
    ScalerRequest nextReq;

    QSharedPointer<QImage> resultImage;
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
            startedRequest = ScalerRequest();
        }
    }

    if (hasNextTask) {
        startRequest(nextReq);
    }

    if (resultImage) {
        emit acceptScalingResult(std::move(resultImage), std::move(resultReq));
    }
}

void Scaler::slotForwardScaledResult(const QSharedPointer<QImage>& image, ScalerRequest req) {
    // ✅ 这里只发生一次真正的像素转换（不可避免）
    QPixmap result = QPixmap::fromImage(*image);

    emit scalingFinished(std::move(result), std::move(req));
}