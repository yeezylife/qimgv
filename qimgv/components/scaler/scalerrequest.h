#ifndef SCALERREQUEST_H
#define SCALERREQUEST_H

#include <QSize>
#include <QMetaType>
#include <memory>
#include "sourcecontainers/image.h"

// 一次 CPU 缩放请求的全部身份信息。
// 路径不单独存一份：Image 已持有唯一权威路径（Image::filePath()，返回常引用），
// 请求里再放一个 QString 只会让每次按值传递多付一次原子引用计数。
class ScalerRequest {
public:
    ScalerRequest() noexcept = default;

    ScalerRequest(std::shared_ptr<Image> image,
                  QSize size,
                  qreal devicePixelRatio,
                  quint64 viewerToken) noexcept
        : m_image(std::move(image))
        , m_size(size)
        , m_devicePixelRatio(devicePixelRatio)
        , m_viewerToken(viewerToken)
    {}

    // ✅ 保持默认语义（Qt6 已优化）
    ScalerRequest(const ScalerRequest&) = default;
    ScalerRequest(ScalerRequest&&) noexcept = default;
    ScalerRequest& operator=(const ScalerRequest&) = default;
    ScalerRequest& operator=(ScalerRequest&&) noexcept = default;

    // 🚀 代数：requestScaled 时递增打戳，用于排队中任务启动前的陈旧性自检
    [[nodiscard]] quint64 generation() const noexcept { return m_generation; }
    void setGeneration(quint64 g) noexcept { m_generation = g; }

    // 视图令牌：由 ImageViewerV2 在换图/重置/切动画/DPR 变化时递增。
    // 结果回到视图时由视图校验，避免旧内容套用到新视图状态。
    [[nodiscard]] quint64 viewerToken() const noexcept { return m_viewerToken; }

    // 目标像素将被显示在什么 dpr 上。worker 产出 QPixmap 时直接打上，
    // 到 GUI 线程后 setDevicePixelRatio() 因同值提前返回，
    // 避免隐式共享解除引发一次整帧深拷贝。
    [[nodiscard]] qreal devicePixelRatio() const noexcept { return m_devicePixelRatio; }

    // 🚀 高频路径：全部返回引用
    [[nodiscard]] const std::shared_ptr<Image>& imageRef() const noexcept {
        return m_image;
    }

    [[nodiscard]] const QSize& sizeRef() const noexcept {
        return m_size;
    }

    // 请求身份 = 同一图像对象 + 同一目标尺寸 + 同一视图令牌（generation 由 Scaler 分配，不参与）
    bool operator==(const ScalerRequest& other) const noexcept {
        return m_image.get() == other.m_image.get() &&
               m_size == other.m_size &&
               m_viewerToken == other.m_viewerToken;
    }

    bool operator!=(const ScalerRequest& other) const noexcept {
        return !(*this == other);
    }

private:
    std::shared_ptr<Image> m_image;
    QSize m_size;
    qreal m_devicePixelRatio = 1.0;
    quint64 m_generation = 0;
    quint64 m_viewerToken = 0;
};

// 任务级结构（非每像素热路径）：上界断言防回归，非精确尺寸（ABI 相关）
// 当前 x86-64 口径 16 + 8 + 8 + 8 + 8 = 48 字节；断言留 8 字节余量，
// 新增成员前先评估必要性，别让它膨胀成多行缓存。
static_assert(sizeof(ScalerRequest) <= 56, "ScalerRequest 体积膨胀：检查新增成员是否必要");

// Qt 元类型（仍然安全）
Q_DECLARE_METATYPE(ScalerRequest)

#endif // SCALERREQUEST_H
