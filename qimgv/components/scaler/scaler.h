#pragma once

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QImage>
#include <QPixmap>
#include <utility> // 增加 std::move 支持
#include "components/cache/cache.h"
#include "scalerrequest.h"
#include "scalerrunnable.h"

class Scaler : public QObject {
    Q_OBJECT
public:
    // explicit 防止隐式转换
    explicit Scaler(Cache *_cache, QObject *parent = nullptr);
    ~Scaler() override;

signals:
    // 信号参数建议保持值传递或常量引用，Qt 内部处理 QueuedConnection 时会自动处理
    void scalingFinished(const QPixmap &result, const ScalerRequest &request);
    void acceptScalingResult(const QImage &image, const ScalerRequest &req);

public slots:
    // 修改为 const &
    void requestScaled(const ScalerRequest &req);

private slots:
    // 修改为 const &
    void onTaskStart(const ScalerRequest &req);
    void onTaskFinish(const QImage &scaled, const ScalerRequest &req);
    void slotForwardScaledResult(const QImage &image, const ScalerRequest &req);

private:
    void startRequest(const ScalerRequest &req);

    QThreadPool *pool;
    Cache *cache;

    bool buffered;
    bool running;
    
    ScalerRequest bufferedRequest;
    ScalerRequest startedRequest;

    mutable QMutex mutex; 
};