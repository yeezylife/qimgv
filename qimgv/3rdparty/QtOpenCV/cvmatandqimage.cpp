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
    default:
        return QImage::Format_ARGB32;
    }
}

// ====================== 通道重排（无拷贝优先） ======================

cv::Mat reorderChannels(const cv::Mat &src, MatColorOrder from, MatColorOrder to) {
    if (from == to) return src; // 🚀 zero-copy

    cv::Mat dst(src.rows, src.cols, src.type());

    if (src.channels() == 4) {
        int map[4];

        if (from == MatColorOrder::ARGB && to == MatColorOrder::BGRA) {
            int tmp[] = {0,3, 1,2, 2,1, 3,0};
            std::copy(tmp, tmp+4, map);
        } else if (from == MatColorOrder::ARGB && to == MatColorOrder::RGBA) {
            int tmp[] = {0,3, 1,0, 2,1, 3,2};
            std::copy(tmp, tmp+4, map);
        } else if (from == MatColorOrder::RGBA && to == MatColorOrder::ARGB) {
            int tmp[] = {0,1, 1,2, 2,3, 3,0};
            std::copy(tmp, tmp+4, map);
        } else {
            // BGRA <-> RGBA
            int tmp[] = {0,2, 1,1, 2,0, 3,3};
            std::copy(tmp, tmp+4, map);
        }

        cv::mixChannels(&src, 1, &dst, 1, map, 4);
        return dst;
    }

    return src;
}

} // namespace

// ====================== QImage → Mat ======================

cv::Mat image2Mat_shared(const QImage &img, MatColorOrder *order) {
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
        break;
    default:
        return cv::Mat();
    }

    return cv::Mat(img.height(), img.width(),
                   CV_8UC(img.depth()/8),
                   const_cast<uchar*>(img.bits()),
                   img.bytesPerLine());
}

cv::Mat image2Mat(const QImage &img, int requiredType, MatColorOrder requiredOrder) {
    if (img.isNull()) return cv::Mat();

    QImage::Format fmt = findClosestFormat(img.format());
    QImage src = (fmt == img.format()) ? img : img.convertToFormat(fmt);

    MatColorOrder srcOrder = MatColorOrder::RGB;
    cv::Mat mat = image2Mat_shared(src, &srcOrder);
    if (mat.empty()) return cv::Mat();

    int targetDepth = CV_MAT_DEPTH(requiredType);
    int targetChannels = CV_MAT_CN(requiredType);
    if (targetChannels == 0) targetChannels = mat.channels();

    // 🚀 fast path
    if (targetDepth == CV_8U &&
        targetChannels == mat.channels() &&
        srcOrder == requiredOrder)
        return mat;

    cv::Mat out = mat;

    // ===== 通道调整 =====
    if (targetChannels != mat.channels()) {
        if (targetChannels == 1) {
            cv::cvtColor(mat, out, cv::COLOR_RGB2GRAY);
        } else if (targetChannels == 3) {
            if (mat.channels() == 4) {
                int map[] = {1,0, 2,1, 3,2};
                out.create(mat.rows, mat.cols, CV_8UC3);
                cv::mixChannels(&mat, 1, &out, 1, map, 3);
            }
        } else if (targetChannels == 4) {
            cv::cvtColor(mat, out, cv::COLOR_RGB2RGBA);
        }
    }

    // ===== 顺序调整 =====
    out = reorderChannels(out, srcOrder, requiredOrder);

    // ===== depth =====
    if (targetDepth != CV_8U) {
        cv::Mat tmp;
        double scale = (targetDepth == CV_16U) ? 255.0 : 1.0 / 255.0;
        out.convertTo(tmp, CV_MAKETYPE(targetDepth, out.channels()), scale);
        return tmp;
    }

    return out;
}

// ====================== Mat → QImage ======================

QImage mat2Image_shared(const cv::Mat &mat, QImage::Format formatHint) {
    if (mat.empty()) return QImage();

    if (mat.type() == CV_8UC3)
        formatHint = QImage::Format_RGB888;
    else if (mat.type() == CV_8UC4)
        formatHint = findClosestFormat(formatHint);
    else
        formatHint = QImage::Format_Indexed8;

    // 🚀 生命周期绑定（关键优化）
    auto matPtr = std::make_shared<cv::Mat>(mat);

    return QImage(mat.data,
                  mat.cols,
                  mat.rows,
                  mat.step,
                  formatHint,
                  [](void *p) {
                      delete static_cast<std::shared_ptr<cv::Mat>*>(p);
                  },
                  new std::shared_ptr<cv::Mat>(matPtr));
}

QImage mat2Image(const cv::Mat &mat, MatColorOrder order, QImage::Format formatHint) {
    if (mat.empty()) return QImage();

    cv::Mat tmp = mat;

    if (mat.channels() == 3 && order == MatColorOrder::BGR) {
        cv::cvtColor(mat, tmp, cv::COLOR_BGR2RGB);
    }

    if (mat.channels() == 4) {
        MatColorOrder required = getColorOrderOfRGB32Format();
        tmp = reorderChannels(mat, order, required);
    }

    if (mat.depth() != CV_8U) {
        cv::Mat conv;
        double scale = (mat.depth() == CV_16U) ? 1.0/255.0 : 255.0;
        tmp.convertTo(conv, CV_8UC(tmp.channels()), scale);
        tmp = conv;
    }

    QImage img = mat2Image_shared(tmp, formatHint);

    if (formatHint == QImage::Format_Invalid)
        return img;

    return img.convertToFormat(formatHint);
}

// ====================== isSupported ======================

bool isSupported(QImage::Format format) {
    return format == QImage::Format_RGB888 ||
           format == QImage::Format_Grayscale8 ||
           format == QImage::Format_RGB32 ||
           format == QImage::Format_ARGB32 ||
           format == QImage::Format_ARGB32_Premultiplied;
}

} // namespace QtOcv