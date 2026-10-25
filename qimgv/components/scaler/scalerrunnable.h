#pragma once

#include <QObject>
#include <QRunnable>
#include <QImage>
#include <atomic>
#include "scalerrequest.h"

class ScalerRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ScalerRunnable(const ScalerRequest& request, std::atomic<quint64>* generation);

    // 保持兼容
    void setRequest(const ScalerRequest& request) { m_request = request; }

    void run() override;

signals:
    // ✅ worker 线程直接产出 QImage（隐式共享），UI 线程零深拷贝。
    // Canvas Painter 的 addImage() 接受 QImage，省掉 QPixmap 往返转换。
    void finished(QImage scaled, ScalerRequest req);

private:
    // 请求是否已被更新的一代取代（阶段边界自检）
    bool outdated() const;

    ScalerRequest m_request;
    std::atomic<quint64>* m_generation;
};