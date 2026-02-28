#ifndef SCALERREQUEST_H
#define SCALERREQUEST_H

#include <QSize>
#include <memory>
#include "sourcecontainers/image.h"
#include "settings.h" // 包含 ScalingFilter 枚举

class ScalerRequest {
public:
    // 默认构造函数
    ScalerRequest() = default;

    // 主构造函数
    ScalerRequest(std::shared_ptr<Image> image, QSize size, QString path, ScalingFilter filter)
        : m_image(std::move(image))
        , m_size(size)
        , m_path(std::move(path))
        , m_filter(filter)
    {}

    // 拷贝和移动构造函数（编译器自动生成即可）
    ScalerRequest(const ScalerRequest&) = default;
    ScalerRequest(ScalerRequest&&) = default;
    ScalerRequest& operator=(const ScalerRequest&) = default;
    ScalerRequest& operator=(ScalerRequest&&) = default;

    // 访问器
    std::shared_ptr<Image> image() const { return m_image; }
    QSize size() const { return m_size; }
    QString path() const { return m_path; }
    ScalingFilter filter() const { return m_filter; }

    // 比较运算符（用于判断缓冲请求是否相同）
    bool operator==(const ScalerRequest& other) const {
        return m_image == other.m_image &&
               m_size == other.m_size &&
               m_filter == other.m_filter;
    }

    bool operator!=(const ScalerRequest& other) const {
        return !(*this == other);
    }

private:
    std::shared_ptr<Image> m_image;
    QSize m_size;
    QString m_path;
    ScalingFilter m_filter;
};

#endif // SCALERREQUEST_H