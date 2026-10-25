#include "scaler.h"
#include <QMutexLocker>

Scaler::Scaler(QObject *parent)
    : QObject(parent)
{
    pool = new QThreadPool(this);
    // 最多同时算两个尺寸：新请求不必等旧请求把整图算完才开始
    pool->setMaxThreadCount(kMaxConcurrent);
}

Scaler::~Scaler() {
    // 先作废所有在算的任务（它们在阶段边界自检代数），并丢弃排队请求：
    // 队列中的过期请求不会启动；已在 QImage::scaled() 内的任务无法中断，
    // waitForDone() 最多等正在执行的一到两次缩放返回
    //（之后的自检会跳过不必要的格式转换）。
    // 必须等待：runnable 持有 mGeneration 指针且 finished 直连到本对象。
    mGeneration.fetch_add(1, std::memory_order_relaxed);
    {
        QMutexLocker locker(&mutex);
        mShuttingDown = true;
        mHasPending = false;
    }
    pool->waitForDone();
}

void Scaler::requestScaled(const ScalerRequest &req) {
    bool start = false;
    ScalerRequest toStart;

    {
        QMutexLocker locker(&mutex);

        // 🚀 每次请求递增代数并打戳，在算/排队中的旧任务据此自检是否已过期。
        // 中间请求只保留最后一次：连续变焦时更早的尺寸已经没人需要
        mPending = req;
        mPending.setGeneration(mGeneration.fetch_add(1, std::memory_order_relaxed) + 1);
        mHasPending = true;

        if (mActive < kMaxConcurrent) {
            toStart = std::move(mPending);
            mHasPending = false;
            ++mActive;
            start = true;
        }
    }

    if (start) {
        startRequest(toStart);
    }
}

void Scaler::startRequest(const ScalerRequest& req) {
    auto *runnable = new ScalerRunnable(req, &mGeneration);
    runnable->setAutoDelete(true);

    // ✅ worker 线程直接产出 QImage，UI 线程零深拷贝
    connect(runnable, &ScalerRunnable::finished,
            this, &Scaler::onTaskFinish,
            Qt::DirectConnection);

    pool->start(runnable);
}

void Scaler::onTaskFinish(QImage scaled, ScalerRequest req) {
    // ⭐ 只有"最新一代"的结果值得交付：更新一代的请求已在路上/已在算，
    // 交付旧尺寸只会让视图来回跳一帧
    const bool current = (req.generation() == mGeneration.load(std::memory_order_relaxed));

    bool start = false;
    ScalerRequest nextReq;

    {
        QMutexLocker locker(&mutex);

        --mActive;

        // 排空"最新请求"槽：并发上限未满就立刻开工
        if (mHasPending && !mShuttingDown && mActive < kMaxConcurrent) {
            nextReq = std::move(mPending);
            mHasPending = false;
            ++mActive;
            start = true;
        }
    }

    if (start) {
        startRequest(nextReq);
    }

    if (current && !scaled.isNull()) {
        emit scalingFinished(std::move(scaled), std::move(req));
    }
}
