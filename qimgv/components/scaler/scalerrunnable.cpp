#include "scalerrunnable.h"
#include "utils/imagelib.h"
#include "settings.h"
#include <utility> // 确保 std::move 可用

ScalerRunnable::ScalerRunnable(const ScalerRequest& request)
    : m_request(request)
{
}

void ScalerRunnable::run()
{
    // 1. 发送开始信号 (由于后续还要用 m_request，这里不能 move)
    emit started(m_request);

    const auto& imageContainer = m_request.imageRef();
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

    // 2. 缩放逻辑
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

    // 3. 发送完成信号
    // 优化：使用 std::move(scaled) 将缩放后的图片转移给接收者
    // 优化：同时 move(m_request)，因为 run 结束了，当前对象的成员已无用
    emit finished(std::move(scaled), std::move(m_request));
}