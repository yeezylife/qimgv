#pragma once

#include <QObject>
#include <QRunnable>
#include <QImage>
#include "scalerrequest.h"

class ScalerRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    // 强制要求传入 request，防止空运行
    explicit ScalerRunnable(const ScalerRequest& request);
    
    // 依然保留 setRequest 以兼容旧逻辑，但建议使用构造函数
    void setRequest(const ScalerRequest& request) { m_request = request; }

    void run() override;

signals:
    // 信号必须携带 ScalerRequest，以便 Scaler 类识别是哪个任务开始了/结束了
    void started(const ScalerRequest& req);
    void finished(QImage scaled, const ScalerRequest& req);

private:
    ScalerRequest m_request;
};