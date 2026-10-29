#pragma once

#include <QObject>
#include <QRunnable>
#include <QPixmap>
#include <atomic>
#include "scalerrequest.h"

class ScalerRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    // 按值接收并 move：请求由任务独占，不再复制一次 shared_ptr
    explicit ScalerRunnable(ScalerRequest request, std::atomic<quint64>* generation);

    void run() override;

signals:
    // worker 线程直接产出 QPixmap；跨线程投递只做隐式共享，UI 线程零深拷贝
    void finished(QPixmap scaled, ScalerRequest req);

private:
    // 请求是否已被更新的一代取代（阶段边界自检）
    bool outdated() const;

    ScalerRequest m_request;
    std::atomic<quint64>* m_generation;
};