#include "scalerrunnable.h"
#include <memory> // for std::unique_ptr (if needed, but not directly used here)

void ScalerRunnable::run()
{
    emit started(m_request);

    // 确定最终使用的滤波算法
    ScalingFilter effectiveFilter = m_request.filter();

    // 如果请求的滤波为 0（可能表示“自动”），则根据缩放方向和设置决定
    if (effectiveFilter == QI_FILTER_AUTO) {  // 假设 QI_FILTER_AUTO 为 0
        const bool isUpscaling = (m_request.size().width() > m_request.image()->width() ||
                                  m_request.size().height() > m_request.image()->height());
        if (isUpscaling && !settings->smoothUpscaling()) {
            effectiveFilter = QI_FILTER_NEAREST;
        } else {
            // 默认使用双线性滤波（可根据需要调整）
            effectiveFilter = QI_FILTER_BILINEAR;
        }
    }

    // 执行缩放，获得新分配的 QImage*（所有权将随信号转移）
    QImage* scaled = ImageLib::scaled(m_request.image()->getImage(),
                                      m_request.size(),
                                      effectiveFilter);

    // 发出完成信号，传递指针所有权
    emit finished(scaled, m_request);
}