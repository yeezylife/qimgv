#pragma once

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QImage>
#include <atomic>
#include "scalerrequest.h"
#include "scalerrunnable.h"

class Scaler : public QObject {
    Q_OBJECT
public:
    explicit Scaler(QObject *parent = nullptr);
    ~Scaler() override;

signals:
    // ✅ QImage 由 worker 线程产出（隐式共享），经 queued 连接浅拷贝送达 UI 线程
    void scalingFinished(QImage result, ScalerRequest request);

public slots:
    void requestScaled(const ScalerRequest &req);

private slots:
    void onTaskFinish(QImage scaled, ScalerRequest req);

private:
    void startRequest(const ScalerRequest &req);

    QThreadPool *pool;

    // ⭐ "最新请求胜出"队列：中间请求只保留最后一次（连续变焦时旧尺寸已无意义）。
    // 并发上限 2（去掉原先 1 线程 + buffered/running 单槽状态机）：
    // 新请求不必排队等旧请求把整图算完，配合 generation 过滤丢弃过期结果，
    // 让最终尺寸的结果尽早出现。
    static constexpr int kMaxConcurrent = 2;
    int mActive = 0;
    ScalerRequest mPending;
    bool mHasPending = false;
    // 析构期间不再启动排队任务（任务对象持有 &mGeneration，必须保证池内任务已排空）
    bool mShuttingDown = false;

    // 🚀 请求代数计数：requestScaled 时递增打戳，排队中/在算的任务据此自检是否已过期
    std::atomic<quint64> mGeneration{0};

    mutable QMutex mutex;
};