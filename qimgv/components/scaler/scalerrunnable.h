#pragma once

#include <QObject>
#include <QRunnable>
#include <QThread>
#include <QDebug>
#include "components/cache/cache.h"
#include "scalerrequest.h"
#include "utils/imagelib.h"
#include "settings.h"

class ScalerRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ScalerRunnable();
    void setRequest(const ScalerRequest& r);
    void run() override;

signals:
    void started(const ScalerRequest&);
    void finished(QImage scaled, const ScalerRequest&);

private:
    ScalerRequest req;
    static constexpr float CMPL_FALLBACK_THRESHOLD = 70.0; // equivalent of ~ 5000x3500 @ 32bpp
};