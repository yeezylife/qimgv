#include "scalerrunnable.h"
#include <memory>
#include "settings.h"

void ScalerRunnable::run()
{
    emit started(m_request);

    auto imageContainer = m_request.image();
    if (!imageContainer)
        return;

    auto imgPtr = imageContainer->getImage();
    if (!imgPtr || imgPtr->isNull())
        return;

    const QImage& sourceImage = *imgPtr;

    // 确定最终使用的滤波算法
    ScalingFilter effectiveFilter = m_request.filter();

    bool useNearest =
        (effectiveFilter == QI_FILTER_NEAREST) ||
        ((m_request.size().width()  > sourceImage.width() ||
          m_request.size().height() > sourceImage.height()) &&
         !settings->smoothUpscaling());

    ScalingFilter filterToUse = useNearest ? QI_FILTER_NEAREST : effectiveFilter;

    // 这里解引用 shared_ptr，避免额外构造
    QImage scaled = ImageLib::scaled(sourceImage,
                                     m_request.size(),
                                     filterToUse);

    emit finished(scaled, m_request);
}