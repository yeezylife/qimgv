#pragma once

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QImage>
#include <QPixmap>
#include <utility> 
#include "components/cache/cache.h"
#include "scalerrequest.h"
#include "scalerrunnable.h"

class Scaler : public QObject {
    Q_OBJECT
public:
    explicit Scaler(Cache *_cache, QObject *parent = nullptr);
    ~Scaler() override;

signals:
    // 核心修改：改为值传递，以便支持 std::move
    void scalingFinished(QPixmap result, ScalerRequest request);
    void acceptScalingResult(QImage image, ScalerRequest req);

public slots:
    void requestScaled(const ScalerRequest &req);

private slots:
    void onTaskStart(const ScalerRequest &req);
    // 核心修改：接收端也改为值传递
    void onTaskFinish(QImage scaled, ScalerRequest req);
    void slotForwardScaledResult(QImage image, ScalerRequest req);

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