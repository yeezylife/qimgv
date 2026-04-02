#include "cvmatandqimage.h"
#include <opencv2/imgproc.hpp>
#include <memory>

namespace QtOcv {
namespace {

// ====================== 工具函数 ======================

constexpr MatColorOrder getColorOrderOfRGB32Format() noexcept {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    return MatColorOrder::BGRA;
#else
    return MatColorOrder::ARGB;
#endif
}

QImage::Format findClosestFormat(QImage::Format formatHint) {
    switch (formatHint) {
    case QImage::Format_Indexed8:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGB888:
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_Alpha8:
    case QImage::Format_Grayscale8:
        return formatHint;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        return QImage::Format_Indexed8;
    case QImage::Format_RGB16:
        return QImage::Format_RGB32;
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB666:
        return QImage::Format_RGB888;
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
        return QImage::Format_ARGB32_Premultiplied;
    default:
        return QImage::Format_ARGB32;
    }
}

// ====================== 通道重排（零拷贝优先） ======================

cv::Mat reorderChannels(const cv::Mat &src, MatColorOrder from, MatColorOrder to) {
    if (from == to) return src; // 🚀 zero-copy

    cv::Mat dst(src.rows, src.cols, src.type());

    if (src.channels() == 4) {
        if (from == MatColorOrder::ARGB && to == MatColorOrder::BGRA) {
            int map[] = {0,3, 1,2, 2,1, 3,0};
            cv::mixChannels(&src, 1, &dst, 1, map, 4);
        } else if (from == MatColorOrder::ARGB && to == MatColorOrder::RGBA) {
            int map[] = {0,3, 1,0, 2,1, 3,2};
            cv::mixChannels(&src, 1, &dst, 1, map, 4);
        } else if (from == MatColorOrder::RGBA && to == MatColorOrder::ARGB) {
            int map[] = {0,1, 1,2, 2,3, 3,0};
            cv::mixChannels(&src, 1, &dst, 1, map, 4);
        } else {
            // BGRA <-> RGBA
            int map[] = {0,2, 1,1, 2,0, 3,3};
            cv::mixChannels(&src, 1, &dst, 1, map, 4);
        }
        return dst;
    }

    if (src.channels() == 3) {
        if ((from == MatColorOrder::RGB && to == MatColorOrder::BGR) ||
            (from == MatColorOrder::BGR && to == MatColorOrder::RGB)) {
            cv::cvtColor(src, dst, cv::COLOR_RGB2BGR);
        } else {
            dst = src.clone(); 
        }
        return dst;
    }

    return src;
}

} // namespace

// ====================== QImage → Mat ======================

cv::Mat image2Mat_shared(const QImage &img, MatColorOrder *order) noexcept {
    if (img.isNull()) return cv::Mat();
    switch (img.format()) {
    case QImage::Format_RGB888:
        if (order) *order = MatColorOrder::RGB;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        if (order) *order = getColorOrderOfRGB32Format();
        break;
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
        if (order) *order = MatColorOrder::RGBA;
        break;
    case QImage::Format_Grayscale8:
    case QImage::Format_Alpha8:
    case QImage::Format_Indexed8:
        // 单通道图像，order 维持未定义，调用者不应依赖
        break;
    default:
        return cv::Mat();
    }
    return cv::Mat(img.height(), img.width(), CV_8UC(img.depth()/8), const_cast<uchar*>(img.bits()), img.bytesPerLine());
}

cv::Mat image2Mat(const QImage &img, int requiredType, MatColorOrder requiredOrder) noexcept {
    if (img.isNull()) return cv::Mat();

    // 1. 支持的格式直接走零拷贝路径，跳过 convertToFormat
    QImage::Format fmt = img.format();
    bool isSupportedFormat = (fmt == QImage::Format_RGB888
        || fmt == QImage::Format_RGB32 || fmt == QImage::Format_ARGB32
        || fmt == QImage::Format_ARGB32_Premultiplied
        || fmt == QImage::Format_RGBX8888 || fmt == QImage::Format_RGBA8888
        || fmt == QImage::Format_RGBA8888_Premultiplied
        || fmt == QImage::Format_Grayscale8 || fmt == QImage::Format_Alpha8
        || fmt == QImage::Format_Indexed8);

    MatColorOrder srcOrder = MatColorOrder::RGB;
    cv::Mat mat;
    if (isSupportedFormat) {
        mat = image2Mat_shared(img, &srcOrder);
    } else {
        QImage src = img.convertToFormat(findClosestFormat(fmt));
        mat = image2Mat_shared(src, &srcOrder);
    }
    if (mat.empty()) return cv::Mat();

    // 3. 解析目标类型
    int targetDepth = CV_MAT_DEPTH(requiredType);
    int targetChannels = CV_MAT_CN(requiredType);
    if (targetChannels == 0) targetChannels = mat.channels();

    // 4. 快速路径：若完全匹配（深度、通道、顺序）则直接返回共享 Mat
    if (targetDepth == CV_8U && targetChannels == mat.channels() && srcOrder == requiredOrder) return mat;

    // 5. 准备中间变量
    cv::Mat out;
    const float maxAlpha = (targetDepth == CV_8U) ? 255.0f : (targetDepth == CV_16U) ? 65535.0f : 1.0f;

    // ==================== 6. 通道数调整 ====================
    if (targetChannels != mat.channels()) {
        if (targetChannels == 1) {
            // 转换为单通道灰度
            if (mat.channels() == 1) {
                out = mat;
            } else if (mat.channels() == 3) {
                cv::cvtColor(mat, out, cv::COLOR_RGB2GRAY);
            } else {
                // 4 通道
                if (srcOrder == MatColorOrder::BGRA) {
                    cv::cvtColor(mat, out, cv::COLOR_BGRA2GRAY);
                } else if (srcOrder == MatColorOrder::RGBA) {
                    cv::cvtColor(mat, out, cv::COLOR_RGBA2GRAY);
                } else {
                    // ARGB: 先提取 RGB 再转灰度
                    cv::Mat rgb(mat.rows, mat.cols, CV_MAKE_TYPE(mat.depth(), 3));
                    int map[] = {1,0, 2,1, 3,2}; // ARGB -> RGB
                    cv::mixChannels(&mat, 1, &rgb, 1, map, 3);
                    cv::cvtColor(rgb, out, cv::COLOR_RGB2GRAY);
                }
            }
        } else if (targetChannels == 3) {
            // 转换为 3 通道 BGR/RGB
            if (mat.channels() == 1) {
                int code = (requiredOrder == MatColorOrder::BGR) ? cv::COLOR_GRAY2BGR : cv::COLOR_GRAY2RGB;
                cv::cvtColor(mat, out, code);
                srcOrder = requiredOrder;
            } else if (mat.channels() == 3) {
                out = mat;
            } else {
                // 4 通道 -> 3 通道（移除 Alpha）
                out = cv::Mat(mat.rows, mat.cols, CV_MAKE_TYPE(mat.depth(), 3));
                if (srcOrder == MatColorOrder::ARGB) {
                    if (requiredOrder == MatColorOrder::BGR) {
                        int map[] = {1,2, 2,1, 3,0};
                        cv::mixChannels(&mat, 1, &out, 1, map, 3);
                    } else {
                        int map[] = {1,0, 2,1, 3,2};
                        cv::mixChannels(&mat, 1, &out, 1, map, 3);
                    }
                } else if (srcOrder == MatColorOrder::BGRA) {
                    if (requiredOrder == MatColorOrder::BGR) {
                        int map[] = {0,0, 1,1, 2,2};
                        cv::mixChannels(&mat, 1, &out, 1, map, 3);
                    } else {
                        int map[] = {2,0, 1,1, 0,2};
                        cv::mixChannels(&mat, 1, &out, 1, map, 3);
                    }
                } else {
                    if (requiredOrder == MatColorOrder::BGR) {
                        int map[] = {2,0, 1,1, 0,2};
                        cv::mixChannels(&mat, 1, &out, 1, map, 3);
                    } else {
                        int map[] = {0,0, 1,1, 2,2};
                        cv::mixChannels(&mat, 1, &out, 1, map, 3);
                    }
                }
                srcOrder = requiredOrder;
            }
        } else if (targetChannels == 4) {
            // 转换为 4 通道 ARGB/RGBA/BGRA
            if (mat.channels() == 1) {
                cv::Mat alpha(mat.rows, mat.cols, mat.type(), cv::Scalar(maxAlpha));
                out = cv::Mat(mat.rows, mat.cols, CV_MAKE_TYPE(mat.type(), 4));
                cv::Mat in[] = {alpha, mat};
                if (requiredOrder == MatColorOrder::ARGB) {
                    int map[] = {0,0, 1,1, 1,2, 1,3};
                    cv::mixChannels(in, 2, &out, 1, map, 4);
                } else if (requiredOrder == MatColorOrder::RGBA) {
                    cv::cvtColor(mat, out, cv::COLOR_GRAY2RGBA);
                } else {
                    cv::cvtColor(mat, out, cv::COLOR_GRAY2BGRA);
                }
                srcOrder = requiredOrder;
            } else if (mat.channels() == 3) {
                cv::Mat alpha(mat.rows, mat.cols, mat.type(), cv::Scalar(maxAlpha));
                out = cv::Mat(mat.rows, mat.cols, CV_MAKE_TYPE(mat.type(), 4));
                cv::Mat in[] = {alpha, mat};
                if (requiredOrder == MatColorOrder::ARGB) {
                    int map[] = {0,0, 1,1, 2,2, 3,3};
                    cv::mixChannels(in, 2, &out, 1, map, 4);
                } else if (requiredOrder == MatColorOrder::RGBA) {
                    cv::cvtColor(mat, out, cv::COLOR_RGB2RGBA);
                } else {
                    cv::cvtColor(mat, out, cv::COLOR_RGB2BGRA);
                }
                srcOrder = requiredOrder;
            } else {
                out = mat;
            }
        }
    } else {
        out = mat;
    }

    // ==================== 7. 顺序调整 ====================
    if (targetChannels == mat.channels() && targetChannels != 1) {
        out = reorderChannels(out, srcOrder, requiredOrder);
    }

    // ==================== 8. 深度转换 ====================
    if (targetDepth != CV_8U) {
        cv::Mat depthMat;
        double scale = (targetDepth == CV_16U) ? 255.0 : 1.0 / 255.0;
        out.convertTo(depthMat, CV_MAKE_TYPE(targetDepth, out.channels()), scale);
        return depthMat;
    }
    return out;
}

// ====================== Mat → QImage ======================

QImage mat2Image_shared(const cv::Mat &mat, QImage::Format formatHint) noexcept {
    if (mat.empty()) return QImage();

    QImage::Format finalFormat = formatHint;
    if (mat.type() == CV_8UC3) {
        finalFormat = QImage::Format_RGB888;
    } else if (mat.type() == CV_8UC4) {
        finalFormat = findClosestFormat(formatHint);
    } else {
        if (formatHint != QImage::Format_Indexed8 && formatHint != QImage::Format_Alpha8 && formatHint != QImage::Format_Grayscale8) {
            finalFormat = QImage::Format_Indexed8;
        } else {
            finalFormat = formatHint;
        }
    }

    // 🚀 使用 nothrow new，确保 noexcept 函数内不会因内存不足抛异常导致崩溃
    cv::Mat* matPtr = new(std::nothrow) cv::Mat(mat);
    if (!matPtr) return QImage();

    QImage img(matPtr->data, 
               matPtr->cols, 
               matPtr->rows, 
               static_cast<qsizetype>(matPtr->step), 
               finalFormat, 
               [](void* p) {
                   delete static_cast<cv::Mat*>(p); 
               }, 
               matPtr);

    if (finalFormat == QImage::Format_Indexed8) {
        QVector<QRgb> colorTable;
        for (int i = 0; i < 256; ++i) colorTable.append(qRgb(i, i, i));
        img.setColorTable(colorTable);
    }

    return img;
}

QImage mat2Image(const cv::Mat &mat, MatColorOrder order, QImage::Format formatHint) noexcept {
    if (mat.empty()) return QImage();
    cv::Mat tmp;

    // ===== 通道数及顺序调整 =====
    if (mat.channels() == 1) {
        tmp = mat;
    } else if (mat.channels() == 3) {
        if (order == MatColorOrder::BGR) {
            cv::cvtColor(mat, tmp, cv::COLOR_BGR2RGB);
        } else {
            tmp = mat;
        }
    } else {
        MatColorOrder requiredOrder = getColorOrderOfRGB32Format();
        if (formatHint == QImage::Format_RGBX8888 || formatHint == QImage::Format_RGBA8888 || formatHint == QImage::Format_RGBA8888_Premultiplied) {
            requiredOrder = MatColorOrder::RGBA;
        }
        if (order == requiredOrder) {
            tmp = mat;
        } else {
            tmp = reorderChannels(mat, order, requiredOrder);
        }
    }

    // ===== 深度转换 =====
    if (tmp.depth() != CV_8U) {
        cv::Mat conv;
        double scale = (tmp.depth() == CV_16U) ? 1.0/255.0 : 255.0;
        tmp.convertTo(conv, CV_8UC(tmp.channels()), scale);
        tmp = conv;
    }

    QImage img = mat2Image_shared(tmp, formatHint);

    if (formatHint != QImage::Format_Invalid && img.format() != formatHint)
        img = img.convertToFormat(formatHint);

    return img;
}

// ====================== isSupported ======================

bool isSupported(QImage::Format format) noexcept {
    return format == QImage::Format_RGB888 || format == QImage::Format_Grayscale8 || format == QImage::Format_RGB32 || format == QImage::Format_ARGB32 || format == QImage::Format_ARGB32_Premultiplied;
}

} // namespace QtOcv