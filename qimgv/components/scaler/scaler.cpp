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

    connect(this, &Scaler::acceptScalingResult,
            this, &Scaler::slotForwardScaledResult,
            Qt::QueuedConnection);
}

Scaler::~Scaler() {
    pool->waitForDone();
}

void Scaler::requestScaled(const ScalerRequest &req) {
    bool needImmediateStart = false;

    // ✅ 仅在需要启动任务时才拷贝
    ScalerRequest startReq;

    {
        QMutexLocker locker(&mutex);

        if (!running) {
            bufferedRequest = req;  // ⚠️ 必须 copy，保证后续比较正确

            if (!buffered) {
                buffered = true;
                needImmediateStart = true;

                // ✅ 只在这里拷贝一次（避免重复 copy）
                startReq = req;
            }
        } else {
            bufferedRequest = req;

            if (!buffered) {
                buffered = true;
            }
        }
    }

    if (needImmediateStart) {
        startRequest(startReq);  // ✅ 用独立副本启动
    }
}

void Scaler::startRequest(const ScalerRequest& req) {
    auto *runnable = new ScalerRunnable(req);
    runnable->setAutoDelete(true);

    connect(runnable, &ScalerRunnable::started,
            this, &Scaler::onTaskStart,
            Qt::DirectConnection);

    connect(runnable, &ScalerRunnable::finished,
            this, &Scaler::onTaskFinish,
            Qt::DirectConnection);

    pool->start(runnable);
}

void Scaler::onTaskStart(const ScalerRequest &req) {
    QMutexLocker locker(&mutex);
    running = true;

    // ✅ 保持原语义（不能动）
    if (buffered && bufferedRequest == req) {
        buffered = false;
    }

    startedRequest = req;  // ⚠️ 这里也必须 copy
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

            // ✅ 这里只做一次 copy（不可避免）
            nextReq = bufferedRequest;
        } else {
            // ✅ 关键优化：全部 move，避免 QImage 深拷贝
            resultImage = std::move(scaled);
            resultReq = std::move(req);

            // 释放引用
            startedRequest = ScalerRequest();
        }
    }

    if (hasNextTask) {
        startRequest(nextReq);
    }

    if (!resultImage.isNull()) {
        // ✅ move 到信号（Qt6 完全支持）
        emit acceptScalingResult(std::move(resultImage), std::move(resultReq));
    }
}

void Scaler::slotForwardScaledResult(QImage image, ScalerRequest req) {
    // ✅ QImage -> QPixmap 这里 move 非常关键
    QPixmap result = QPixmap::fromImage(std::move(image));

    emit scalingFinished(std::move(result), std::move(req));
}