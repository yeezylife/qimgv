#include "scalerrunnable.h"
#include "utils/imagelib.h"
#include <utility>

ScalerRunnable::ScalerRunnable(const ScalerRequest& request, std::atomic<quint64>* generation)
    : m_request(request),
      m_generation(generation)
{
}

void ScalerRunnable::run()
{
    // 🚀 陈旧性自检：排队期间若有更新的请求到来（代数已递增），直接放弃，避免白算一次缩放
    if (outdated())
    {
        emit finished(QPixmap(), std::move(m_request));
        return;
    }

    const auto& imageContainer = m_request.imageRef();
    if (!imageContainer)
    {
        emit finished(QPixmap(), std::move(m_request));
        return;
    }

    auto imgPtr = imageContainer->getImage();
    if (!imgPtr || imgPtr->isNull())
    {
        emit finished(QPixmap(), std::move(m_request));
        return;
    }

    // 2️⃣ 缩放（统一使用 Qt 平滑变换）
    const QImage& sourceImage = *imgPtr;
    const QSize& targetSize = m_request.sizeRef();

    QImage scaled = ImageLib::scaled(sourceImage, targetSize);

    // 🚀 缩放结束后再自检一次：单次 resize 调用内部无法中断，
    // 但后续的格式转换 + QPixmap 构造（整图像素拷贝）可以省掉
    if (outdated())
    {
        emit finished(QPixmap(), std::move(m_request));
        return;
    }

    // 3️⃣ 🚀 关键优化：像素转换移入 worker 线程（Windows raster 允许在非 GUI 线程创建 QPixmap），
    //    UI 线程只做隐式共享浅拷贝。已是 ARGB32_Premultiplied 时跳过格式转换。
    QPixmap result = (scaled.format() == QImage::Format_ARGB32_Premultiplied)
                         ? QPixmap::fromImage(scaled, Qt::NoFormatConversion)
                         : QPixmap::fromImage(scaled);

    // m_request 生命周期结束 → move
    emit finished(std::move(result), std::move(m_request));
}

bool ScalerRunnable::outdated() const
{
    return m_generation
           && m_generation->load(std::memory_order_relaxed) != m_request.generation();
}
