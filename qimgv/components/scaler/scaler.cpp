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
    }  // 【优化点1】锁在此释放，后续操作在锁外执行

    // 【优化点1】cache 操作移出锁外（I/O 可能耗时）
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
    
    // 【优化点2】用于锁外发射信号的临时变量
    QImage resultImage;
    ScalerRequest resultReq;

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
            // 【优化点2】将结果移动到临时变量，锁外发射信号
            resultImage = std::move(scaled);
            resultReq = std::move(req);
        }
    }  // 【优化点2】锁在此释放，后续所有操作在锁外执行

    // 【优化点2】以下操作均在锁外执行，减少锁持有时间
    
    // I/O 操作（可能涉及文件系统，耗时）
    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    // 启动下一个任务（可能涉及线程调度）
    if (hasNextTask) {
        startRequest(nextReq);
    }

    // 信号发射（可能触发多个槽函数，耗时不确定）
    if (!resultImage.isNull()) {
        emit acceptScalingResult(std::move(resultImage), std::move(resultReq));
    }
}

void Scaler::slotForwardScaledResult(QImage image, ScalerRequest req) {
    // QPixmap::fromImage 在 Qt6 中针对右值有优化
    QPixmap result = QPixmap::fromImage(std::move(image));
    // 彻底消除警告，实现真正的资源转移
    emit scalingFinished(std::move(result), std::move(req));
}