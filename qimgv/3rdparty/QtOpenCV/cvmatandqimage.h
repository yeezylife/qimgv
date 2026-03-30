#ifndef CVMATANDQIMAGE_H
#define CVMATANDQIMAGE_H

#include <QtGui/qimage.h>
#include <opencv2/core.hpp>

namespace QtOcv {

enum class MatColorOrder {
    BGR,
    RGB,
    BGRA = BGR,
    RGBA = RGB,
    ARGB
};


/* ====================== 深拷贝版本 ======================
 *
 * 安全版本：
 * - 返回的 cv::Mat / QImage 拥有独立数据
 * - 不依赖输入对象生命周期
 *
 * 支持：
 * - channels: 1 / 3 / 4
 * - depth: CV_8U / CV_16U / CV_32F
 */
cv::Mat image2Mat(const QImage &img,
                  int requiredMatType = CV_8UC(0),
                  MatColorOrder requiredOrder = MatColorOrder::BGR) noexcept;

QImage mat2Image(const cv::Mat &mat,
                 MatColorOrder order = MatColorOrder::BGR,
                 QImage::Format formatHint = QImage::Format_Invalid) noexcept;


/* ====================== 共享内存版本（零拷贝） ======================
 *
 * ⚠️ 高性能接口（需注意生命周期）
 *
 * image2Mat_shared：
 *   - 返回 Mat 直接引用 QImage 内存
 *   - ⚠️ QImage 必须在 Mat 使用期间保持存活
 *
 * mat2Image_shared：
 *   - 返回 QImage 与 Mat 共享内存
 *   - ✔ 已内部绑定 shared_ptr，安全
 *
 * 支持格式：
 *   - QImage::Format_Indexed8      <==> CV_8UC1
 *   - QImage::Format_Grayscale8    <==> CV_8UC1
 *   - QImage::Format_RGB888        <==> CV_8UC3
 *   - QImage::Format_ARGB32 等     <==> CV_8UC4
 *   - QImage::Format_RGBA8888 等   <==> CV_8UC4
 */
cv::Mat image2Mat_shared(const QImage &img,
                         MatColorOrder *order = nullptr) noexcept;

QImage mat2Image_shared(const cv::Mat &mat,
                        QImage::Format formatHint = QImage::Format_Invalid) noexcept;


/* ====================== 工具 ====================== */

bool isSupported(QImage::Format format) noexcept;

} // namespace QtOcv

#endif // CVMATANDQIMAGE_H