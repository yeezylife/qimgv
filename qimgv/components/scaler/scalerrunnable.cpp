#include "scalerrunnable.h"
#include <memory>
#include "settings.h"

void ScalerRunnable::run()
{
    emit started(m_request);

    auto imageContainer = m_request.image();
    if (!imageContainer)
    {
        emit finished(QImage(), m_request);
        return;
    }

    auto imgPtr = imageContainer->getImage();
    if (!imgPtr || imgPtr->isNull())
    {
        emit finished(QImage(), m_request);
        return;
    }

    const QImage& sourceImage = *imgPtr;

    ScalingFilter effectiveFilter = m_request.filter();

    bool useNearest =
        (effectiveFilter == QI_FILTER_NEAREST) ||
        ((m_request.size().width()  > sourceImage.width() ||
          m_request.size().height() > sourceImage.height()) &&
         !settings->smoothUpscaling());

    ScalingFilter filterToUse = useNearest ? QI_FILTER_NEAREST : effectiveFilter;

    QImage scaled = ImageLib::scaled(sourceImage,
                                     m_request.size(),
                                     filterToUse);

    emit finished(scaled, m_request);
}