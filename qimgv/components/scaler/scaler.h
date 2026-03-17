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
    // 值传递，支持 std::move 避免拷贝
    void scalingFinished(QPixmap result, ScalerRequest request);
    void acceptScalingResult(QImage image, ScalerRequest req);

public slots:
    void requestScaled(const ScalerRequest &req);

private slots:
    void onTaskStart(const ScalerRequest &req);
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