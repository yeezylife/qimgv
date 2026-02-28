#pragma once

#include <QObject>
#include <QRunnable>
#include "scalerrequest.h"
#include "utils/imagelib.h"
#include "settings.h"

class ScalerRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ScalerRunnable() = default;
    void setRequest(const ScalerRequest& request) { m_request = request; }

    void run() override;

signals:
    void started(const ScalerRequest&);
    // 注意：finished 传递 QImage*，所有权转移给接收者，接收者必须负责 delete
    void finished(QImage* scaled, const ScalerRequest&);

private:
    ScalerRequest m_request;
};