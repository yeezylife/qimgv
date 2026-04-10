#include "scalerrunnable.h"
#include "utils/imagelib.h"
#include "settings.h"
#include <utility>

ScalerRunnable::ScalerRunnable(const ScalerRequest& request)
    : m_request(request)
{
}

void ScalerRunnable::run()
{
    // 1️⃣ 开始信号（不能 move）
    emit started(m_request);

    const auto& imageContainer = m_request.imageRef();
    if (!imageContainer)
    {
        emit finished(QSharedPointer<QImage>(), m_request);
        return;
    }

    auto imgPtr = imageContainer->getImage();
    if (!imgPtr || imgPtr->isNull())
    {
        emit finished(QSharedPointer<QImage>(), m_request);
        return;
    }

    // 2️⃣ 缩放
    const QImage& sourceImage = *imgPtr;
    ScalingFilter effectiveFilter = m_request.filter();
    const QSize& targetSize = m_request.sizeRef();

    bool useNearest = (effectiveFilter == QI_FILTER_NEAREST) ||
                      ((targetSize.width()  > sourceImage.width() ||
                        targetSize.height() > sourceImage.height()) &&
                       !settings->smoothUpscaling());

    ScalingFilter filterToUse = useNearest ? QI_FILTER_NEAREST : effectiveFilter;

    QImage scaled = ImageLib::scaled(sourceImage,
                                     targetSize,
                                     filterToUse);

    // 3️⃣ 🚀 关键优化：直接转为 shared pointer（零额外拷贝）
    auto sharedImage = QSharedPointer<QImage>::create(std::move(scaled));

    // m_request 生命周期结束 → move
    emit finished(std::move(sharedImage), std::move(m_request));
}