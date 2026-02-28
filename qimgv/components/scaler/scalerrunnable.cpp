#include "scalerrunnable.h"
#include "settings.h"

void ScalerRunnable::run()
{
    emit started(m_request);

    ScalingFilter filterToUse;
    bool useNearest = (m_request.filter() == QI_FILTER_NEAREST) ||
                      ( (m_request.size().width() > m_request.image()->width() ||
                         m_request.size().height() > m_request.image()->height()) &&
                        !settings->smoothUpscaling() );

    if (useNearest) {
        filterToUse = QI_FILTER_NEAREST;
    } else {
        filterToUse = m_request.filter();
    }

    QImage* scaled = ImageLib::scaled(m_request.image()->getImage(),
                                      m_request.size(),
                                      filterToUse);
    emit finished(scaled, m_request);
}