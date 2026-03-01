#include "scalerrunnable.h"
#include <memory>
#include "settings.h"

void ScalerRunnable::run()
{
    emit started(m_request);

    // 确定最终使用的滤波算法
    ScalingFilter effectiveFilter = m_request.filter();
    bool useNearest = (effectiveFilter == QI_FILTER_NEAREST) ||
                      ( (m_request.size().width() > m_request.image()->width() ||
                         m_request.size().height() > m_request.image()->height()) &&
                        !settings->smoothUpscaling() );

    ScalingFilter filterToUse = useNearest ? QI_FILTER_NEAREST : effectiveFilter;

    // 调用 ImageLib::scaled，按值返回 QImage
    QImage scaled = ImageLib::scaled(m_request.image()->getImage(), m_request.size(), filterToUse);
    emit finished(scaled, m_request);
}