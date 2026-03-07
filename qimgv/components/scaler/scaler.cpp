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
    // 图片查看器通常设为 1，以保证缩放任务按顺序执行，避免争抢 CPU
    pool->setMaxThreadCount(1); 
    
    // 信号转发：确保结果在主线程处理（QueuedConnection）
    connect(this, &Scaler::acceptScalingResult, this, &Scaler::slotForwardScaledResult, Qt::QueuedConnection);
}

Scaler::~Scaler() {
    pool->waitForDone(); // 退出前确保后台任务清理干净
}

void Scaler::requestScaled(ScalerRequest req) {
    QString toReserve;
    QString toRelease;
    bool needImmediateStart = false;

    {
        QMutexLocker locker(&mutex);

        if (!running) {
            if (!buffered) {
                // 1. 完全空闲：直接执行
                bufferedRequest = req;
                buffered = true;
                toReserve = req.image()->fileName();
                needImmediateStart = true;
            } else {
                // 2. 之前有缓冲但还没来得及跑（罕见竞态）：替换掉旧的
                if (bufferedRequest.image() != req.image()) {
                    toRelease = bufferedRequest.image()->fileName();
                    toReserve = req.image()->fileName();
                }
                bufferedRequest = req;
            }
        } else {
            // 3. 正在运行中：更新缓冲区
            if (!buffered) {
                bufferedRequest = req;
                buffered = true;
                if (req.image() != startedRequest.image()) {
                    toReserve = req.image()->fileName();
                }
            } else {
                // 替换掉已经在缓冲但尚未开始的任务
                if (bufferedRequest.image() != req.image()) {
                    // 如果缓冲的和正在跑的不是同一个图，且新图也不同，则释放旧缓冲
                    if (bufferedRequest.image() != startedRequest.image()) {
                        toRelease = bufferedRequest.image()->fileName();
                    }
                    if (req.image() != startedRequest.image()) {
                        toReserve = req.image()->fileName();
                    }
                    bufferedRequest = req;
                } else {
                    bufferedRequest = req; // 参数更新（比如缩放比例变了）
                }
            }
        }
    }

    // 资源锁定/释放放在锁外，避免阻塞逻辑
    if (!toReserve.isEmpty()) cache->reserve(toReserve);
    if (!toRelease.isEmpty()) cache->release(toRelease);

    if (needImmediateStart) {
        startRequest(req);
    }
}

void Scaler::startRequest(const ScalerRequest& req) {
    // 关键修复：每次创建新的 Runnable，避免复用导致的参数覆盖
    auto *runnable = new ScalerRunnable();
    runnable->setAutoDelete(true); // 线程池会自动 delete 它
    runnable->setRequest(req);

    // 绑定回调：使用 DirectConnection 因为回调里会自己加锁，且需保证状态同步
    connect(runnable, &ScalerRunnable::started, this, &Scaler::onTaskStart, Qt::DirectConnection);
    connect(runnable, &ScalerRunnable::finished, this, &Scaler::onTaskFinish, Qt::DirectConnection);

    pool->start(runnable);
}

void Scaler::onTaskStart(ScalerRequest req) {
    QMutexLocker locker(&mutex);
    running = true;
    
    // 如果当前开始的任务正好是缓冲区里那个，清除缓冲标记
    if (buffered && bufferedRequest == req) {
        buffered = false;
    }
    startedRequest = req;
}

void Scaler::onTaskFinish(QImage scaled, ScalerRequest req) {
    QString toRelease;
    bool hasNextTask = false;
    ScalerRequest nextReq;

    {
        QMutexLocker locker(&mutex);
        running = false;

        // 判断是否需要释放缓存：如果后续没人在用这张图了，就释放
        if (!(buffered && bufferedRequest.image() == req.image())) {
            toRelease = req.image()->fileName();
        }

        if (buffered) {
            // 模式 3b：有新请求进来了，放弃当前结果，开启新任务
            hasNextTask = true;
            nextReq = bufferedRequest;
        } else {
            // 模式 3a：没有新请求，把结果发出去
            emit acceptScalingResult(scaled, req);
        }
    }

    if (!toRelease.isEmpty()) {
        cache->release(toRelease);
    }

    if (hasNextTask) {
        startRequest(nextReq);
    }
}

void Scaler::slotForwardScaledResult(QImage image, ScalerRequest req) {
    // QPixmap::fromImage 必须在 GUI 线程（主线程）执行
    QPixmap result = QPixmap::fromImage(image);
    emit scalingFinished(result, req);
}