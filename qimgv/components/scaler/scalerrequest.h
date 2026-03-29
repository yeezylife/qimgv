#ifndef SCALERREQUEST_H
#define SCALERREQUEST_H

#include <QSize>
#include <QString>
#include <memory>
#include "sourcecontainers/image.h"
#include "settings.h" // 包含 ScalingFilter 枚举

class ScalerRequest {
public:
    ScalerRequest() : m_filter(QI_FILTER_BILINEAR) {} // 默认构造

    ScalerRequest(std::shared_ptr<Image> image, QSize size, QString path, ScalingFilter filter)
        : m_image(std::move(image))
        , m_size(size)
        , m_path(std::move(path))
        , m_filter(filter)
    {}

    // 默认拷贝/移动语义在 Qt6/C++17 下表现良好
    ScalerRequest(const ScalerRequest&) = default;
    ScalerRequest(ScalerRequest&&) = default;
    ScalerRequest& operator=(const ScalerRequest&) = default;
    ScalerRequest& operator=(ScalerRequest&&) = default;

    // 访问器（保持兼容接口，新增引用版本供高频路径使用）
    [[nodiscard]] std::shared_ptr<Image> image() const { return m_image; }
    [[nodiscard]] const std::shared_ptr<Image>& imageRef() const noexcept { return m_image; }

    [[nodiscard]] QSize size() const { return m_size; }
    [[nodiscard]] const QSize& sizeRef() const noexcept { return m_size; }

    [[nodiscard]] QString path() const { return m_path; }
    [[nodiscard]] const QString& pathRef() const noexcept { return m_path; }

    [[nodiscard]] ScalingFilter filter() const noexcept { return m_filter; }

    // 核心逻辑：判断两个请求是否在处理同一张图的同一个规格
    bool operator==(const ScalerRequest& other) const noexcept {
        return m_image.get() == other.m_image.get() &&
               m_size == other.m_size &&
               m_filter == other.m_filter;
    }

    bool operator!=(const ScalerRequest& other) const noexcept {
        return !(*this == other);
    }

private:
    std::shared_ptr<Image> m_image;
    QSize m_size;
    QString m_path;
    ScalingFilter m_filter;
};

// 注册到 Qt 类型系统，以便在信号槽中跨线程传递（如果需要）
Q_DECLARE_METATYPE(ScalerRequest)

#endif // SCALERREQUEST_H