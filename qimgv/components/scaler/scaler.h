#pragma once

#include <QObject>
#include <QThreadPool>
#include <QMutex>
#include <QImage>
#include <QPixmap>
#include <QSharedPointer>
#include "components/cache/cache.h"
#include "scalerrequest.h"
#include "scalerrunnable.h"

class Scaler : public QObject {
    Q_OBJECT
public:
    explicit Scaler(Cache *_cache, QObject *parent = nullptr);
    ~Scaler() override;

signals:
    // ✅ 用 QSharedPointer 避免深拷贝
    void scalingFinished(QPixmap result, ScalerRequest request);

    // ✅ 内部传递也用 shared
    void acceptScalingResult(QSharedPointer<QImage> image, ScalerRequest req);

public slots:
    void requestScaled(const ScalerRequest &req);

private slots:
    void onTaskStart(const ScalerRequest &req);

    // ✅ 接收 shared pointer
    void onTaskFinish(QSharedPointer<QImage> scaled, ScalerRequest req);

    void slotForwardScaledResult(const QSharedPointer<QImage>& image, ScalerRequest req);

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