#pragma once

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QImage>
#include <QPixmap>
#include "components/cache/cache.h"
#include "scalerrequest.h"
#include "scalerrunnable.h"

class Scaler : public QObject {
    Q_OBJECT
public:
    explicit Scaler(Cache *_cache, QObject *parent = nullptr);
    ~Scaler() override; // 确保退出时线程安全

signals:
    // 使用值传递，Qt 内部会通过隐式共享（Copy-on-Write）优化，安全且高效
    void scalingFinished(QPixmap result, ScalerRequest request);
    void acceptScalingResult(QImage image, ScalerRequest req);

public slots:
    void requestScaled(ScalerRequest req);

private slots:
    void onTaskStart(ScalerRequest req);
    void onTaskFinish(QImage scaled, ScalerRequest req);
    void slotForwardScaledResult(QImage image, ScalerRequest req);

private:
    void startRequest(const ScalerRequest& req);

    QThreadPool *pool;
    Cache *cache;

    // 状态管理
    bool buffered;
    bool running;
    
    ScalerRequest bufferedRequest;
    ScalerRequest startedRequest;

    mutable QMutex mutex; // 保护状态位和请求变量
};