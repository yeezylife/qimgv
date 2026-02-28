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
    pool->setMaxThreadCount(1);
    runnable = new ScalerRunnable();
    runnable->setAutoDelete(false);
    
    connect(runnable, &ScalerRunnable::started, this, &Scaler::onTaskStart, Qt::DirectConnection);
    connect(runnable, &ScalerRunnable::finished, this, &Scaler::onTaskFinish, Qt::DirectConnection);
    
    // 使用 QueuedConnection 确保图片转换在主线程进行
    connect(this, &Scaler::acceptScalingResult, this, &Scaler::slotForwardScaledResult, Qt::QueuedConnection);
}

void Scaler::requestScaled(ScalerRequest req) {
    QString toReserve;
    QString toRelease;
    bool needStart = false;
    ScalerRequest reqToStart;

    { // 临界区：仅保护内部状态
        QMutexLocker locker(&mutex);

        // === 状态更新逻辑 ===
        if (!running) {
            // 情况 A: 无任务运行
            if (!buffered) {
                // A1: 空闲 -> 启动新任务
                bufferedRequest = req;
                buffered = true;
                toReserve = req.image->fileName();
                needStart = true;
                reqToStart = req;
            } else {
                // A2: 有缓冲 -> 更新缓冲
                if (bufferedRequest.image != req.image) {
                    toRelease = bufferedRequest.image->fileName();
                    toReserve = req.image->fileName();
                    bufferedRequest = req;
                } else {
                    bufferedRequest = req;
                }
            }
        } else {
            // 情况 B: 有任务运行
            if (!buffered) {
                // B1: 无缓冲 -> 创建缓冲
                bufferedRequest = req;
                buffered = true;
                if (req.image != startedRequest.image) {
                    toReserve = req.image->fileName();
                }
            } else {
                // B2: 有缓冲 -> 替换缓冲
                if (bufferedRequest.image != req.image) {
                    if (bufferedRequest.image != startedRequest.image) {
                        toRelease = bufferedRequest.image->fileName();
                    }
                    if (req.image != startedRequest.image) {
                        toReserve = req.image->fileName();
                    }
                    bufferedRequest = req;
                } else {
                    bufferedRequest = req;
                }
            }
        }
    } // 解锁

    // === 锁外执行耗时操作 ===
    if (!toReserve.isEmpty()) {
        cache->reserve(toReserve);
    }
    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    // === 启动任务 ===
    // 必须在 reserve 之后调用，防止任务启动时图片未被锁定
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

void Scaler::onTaskFinish(QImage *scaled, ScalerRequest req) {
    // 使用智能指针管理所有权，确保函数退出时必然删除
    std::unique_ptr<QImage> scaledGuard(scaled);
    
    QString toRelease;
    bool hasBuffered = false;
    ScalerRequest nextReq;

    { // 临界区
        QMutexLocker locker(&mutex);
        running = false;

        // 判断是否需要释放当前图片缓存
        if (buffered && bufferedRequest.image == req.image) {
            // 图片将继续使用，不释放
        } else {
            toRelease = req.image->fileName();
        }

        if (buffered) {
            // 有新请求，丢弃当前结果，准备下次启动
            hasBuffered = true;
            nextReq = bufferedRequest;
        } else {
            // 无新请求，发送结果
            // Qt信号值传递会触发隐式共享拷贝，数据安全
            emit acceptScalingResult(*scaled, req);
            // scaledGuard 将在离开作用域时自动 delete scaled
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
    // 此槽函数通过 QueuedConnection 在主线程执行
    // image 是值传递，拥有独立数据所有权，安全
    QPixmap result = QPixmap::fromImage(image);
    emit scalingFinished(result, req);
}

void Scaler::startRequest(ScalerRequest req) {
    QMutexLocker locker(&mutex);
    // 在任务实际开始前将 running 设为 true，消除窗口期
    running = true;
    runnable->setRequest(req);
    pool->start(runnable);
}