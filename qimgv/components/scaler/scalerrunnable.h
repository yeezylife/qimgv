#pragma once

#include <QObject>
#include <QRunnable>
#include <QImage>
#include "scalerrequest.h"

class ScalerRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ScalerRunnable(const ScalerRequest& request);
    
    // 依然保留以兼容，内部自动处理拷贝
    void setRequest(const ScalerRequest& request) { m_request = request; }

    void run() override;

signals:
    // 修改为值传递，支持移动语义
    void started(ScalerRequest req);
    void finished(QImage scaled, ScalerRequest req);

private:
    ScalerRequest m_request;
};