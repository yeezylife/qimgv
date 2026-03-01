#include "scalerrunnable.h"

ScalerRunnable::ScalerRunnable()
{
}

void ScalerRunnable::setRequest(const ScalerRequest& r)
{
    req = r;
}

void ScalerRunnable::run()
{
    emit started(req);

    // Choose filter based on request and settings
    int filter = req.filter;
    if (filter == 0 || (req.size.width() > req.image->width() && !settings->smoothUpscaling())) {
        filter = QI_FILTER_NEAREST;
    }

    // scaled now returns QImage by value (no pointer)
    QImage scaled = ImageLib::scaled(req.image->getImage(), req.size, filter);
    emit finished(scaled, req);
}