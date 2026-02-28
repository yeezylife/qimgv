#pragma once

#include <QObject>
#include <QThreadPool>
#include <QThread>
#include <QMutex>
#include <memory>
#include "components/cache/cache.h"
#include "scalerrequest.h"
#include "scalerrunnable.h"

class Scaler : public QObject {
    Q_OBJECT
public:
    explicit Scaler(Cache *_cache, QObject *parent = nullptr);

signals:
    // 修改为值传递，由接收者管理生命周期，避免裸指针悬挂
    void scalingFinished(QPixmap result, ScalerRequest request);
    // 内部信号，同样使用值传递
    void acceptScalingResult(QImage image, ScalerRequest req);

public slots:
    void requestScaled(ScalerRequest req);

private slots:
    void onTaskStart(ScalerRequest req);
    void onTaskFinish(QImage* scaled, ScalerRequest req);
    void slotForwardScaledResult(QImage image, ScalerRequest req);

private:
    QThreadPool *pool;
    ScalerRunnable *runnable;
    
    bool buffered;
    bool running;
    
    ScalerRequest bufferedRequest;
    ScalerRequest startedRequest;

    Cache *cache;

    void startRequest(ScalerRequest req);

    mutable QMutex mutex; // 保护 running, buffered, requests, runnable 访问
};
