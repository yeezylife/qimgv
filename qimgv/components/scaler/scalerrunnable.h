#pragma once

#include <QObject>
#include <QRunnable>
#include <QImage>
#include <QSharedPointer>
#include "scalerrequest.h"

class ScalerRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ScalerRunnable(const ScalerRequest& request);

    // 保持兼容
    void setRequest(const ScalerRequest& request) { m_request = request; }

    void run() override;

signals:
    // ✅ 仍然值传递（小对象，没问题）
    void started(ScalerRequest req);

    // ✅ 改为 QSharedPointer，彻底避免深拷贝
    void finished(QSharedPointer<QImage> scaled, ScalerRequest req);

private:
    ScalerRequest m_request;
};