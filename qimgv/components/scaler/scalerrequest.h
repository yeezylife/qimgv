#ifndef SCALERREQUEST_H
#define SCALERREQUEST_H

#include <QSize>
#include <QString>
#include <memory>
#include <utility>
#include "sourcecontainers/image.h"

class ScalerRequest {
public:
    ScalerRequest() noexcept = default;

    ScalerRequest(std::shared_ptr<Image> image,
                  QSize size,
                  QString path,
                  quint64 viewerToken = 0) noexcept
        : m_image(std::move(image))
        , m_size(size)
        , m_path(std::move(path))
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

    // 🚀 高频路径：全部返回引用
    [[nodiscard]] const std::shared_ptr<Image>& imageRef() const noexcept {
        return m_image;
    }

    [[nodiscard]] const QSize& sizeRef() const noexcept {
        return m_size;
    }

    [[nodiscard]] const QString& pathRef() const noexcept {
        return m_path;
    }

    // 视图侧请求令牌：原样带回，用于丢弃快速切图/连续变焦后的过期缩放结果
    [[nodiscard]] quint64 viewerToken() const noexcept { return m_viewerToken; }

    // 🚀 指针比较避免字符串比较（非常关键）；代数不参与相等比较
    bool operator==(const ScalerRequest& other) const noexcept {
        return m_image.get() == other.m_image.get() &&
               m_size == other.m_size;
    }

    bool operator!=(const ScalerRequest& other) const noexcept {
        return !(*this == other);
    }

private:
    std::shared_ptr<Image> m_image;
    QSize m_size;
    QString m_path;
    quint64 m_viewerToken = 0;
    quint64 m_generation = 0;
};

// 任务级结构（非每像素热路径）：上界断言防回归，非精确尺寸（ABI 相关）
static_assert(sizeof(ScalerRequest) <= 64, "ScalerRequest 体积膨胀：检查新增成员是否必要");

// Qt 元类型（仍然安全）
Q_DECLARE_METATYPE(ScalerRequest)

#endif // SCALERREQUEST_H