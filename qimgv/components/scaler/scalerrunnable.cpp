#include "scalerrunnable.h"
#include "utils/imagelib.h"
#include "settings.h"

ScalerRunnable::ScalerRunnable(const ScalerRequest& request)
    : m_request(request)
{
}

void ScalerRunnable::run()
{
    // 1. 发送开始信号
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

    // 2. 缩放逻辑
    const QImage& sourceImage = *imgPtr;
    ScalingFilter effectiveFilter = m_request.filter();

    // 判断是否需要强制使用邻近插值（Nearest Neighbor）
    // 逻辑：如果是放大操作且设置关闭了平滑缩放，则使用邻近插值
    bool useNearest = (effectiveFilter == QI_FILTER_NEAREST) ||
                      ((m_request.size().width()  > sourceImage.width() ||
                        m_request.size().height() > sourceImage.height()) &&
                       !settings->smoothUpscaling());

    ScalingFilter filterToUse = useNearest ? QI_FILTER_NEAREST : effectiveFilter;

    // 执行缩放（底层调用 ImageLib）
    QImage scaled = ImageLib::scaled(sourceImage,
                                     m_request.size(),
                                     filterToUse);

    // 3. 发送完成信号
    // 注意：Qt6 中 QImage 是隐式共享的，传值开销极小
    emit finished(scaled, m_request);
}