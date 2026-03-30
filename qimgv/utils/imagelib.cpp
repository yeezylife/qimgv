#include "imagelib.h"
#include <memory>

#ifdef USE_OPENCV
#include "3rdparty/QtOpenCV/cvmatandqimage.h"
#endif

void ImageLib::recolor(QPixmap &pixmap, const QColor &color) {
    if (pixmap.isNull()) return;
    
    // 优化：使用 RAII 管理 QPainter 生命周期
    {
        QPainter p(&pixmap);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(pixmap.rect(), color);
    }
    // QPainter 在作用域结束时自动销毁，释放资源
}

QImage ImageLib::rotatedRaw(const QImage &src, int grad) {
    if (src.isNull()) return QImage();
    QTransform transform;
    transform.rotate(grad);
    return src.transformed(transform, Qt::SmoothTransformation);
}

QImage ImageLib::rotated(const QImage &src, int grad) {
    if (src.isNull()) return QImage();
    // 旋转角度为 360° 的整数倍，无需变换，直接返回源图像（注意：src 是 const&，返回时会拷贝，但避免了 transform 开销）
    if (grad % 360 == 0) {
        return src;
    }
    return rotatedRaw(src, grad);
}

QImage ImageLib::croppedRaw(const QImage &src, QRect newRect) {
    if (!src.isNull() && src.rect().contains(newRect)) {
        return src.copy(newRect);
    }
    return QImage();
}

QImage ImageLib::cropped(const QImage &src, QRect newRect) {
    if (src.isNull()) return QImage();
    // 裁剪区域等于原图大小，直接返回源图像
    if (src.rect() == newRect) {
        return src;
    }
    return croppedRaw(src, newRect);
}

// --- flipped: 利用 Qt6 的 QImage::flipped() && ---

QImage ImageLib::flippedHRaw(QImage src) {
    if (src.isNull()) return QImage();
    // 关键：std::move(src) 触发 QImage::flipped(Qt::Axis) &&
    return std::move(src).flipped(Qt::Horizontal);
}

QImage ImageLib::flippedH(QImage src) {
    if (src.isNull()) return QImage();
    return flippedHRaw(std::move(src));
}

QImage ImageLib::flippedVRaw(QImage src) {
    if (src.isNull()) return QImage();
    return std::move(src).flipped(Qt::Vertical);
}

QImage ImageLib::flippedV(QImage src) {
    if (src.isNull()) return QImage();
    return flippedVRaw(std::move(src));
}

// --- EXIF 旋转：transformed 没有 && 版本，只能深拷贝 ---

QImage ImageLib::exifRotated(QImage src, int orientation) {
    if (src.isNull() || orientation <= 1) return src;

    QTransform trans;
    bool needsTransform = true;

    switch (orientation) {
        case 2: trans.scale(-1, 1); break;
        case 3: trans.rotate(180); break;
        case 4: trans.scale(1, -1); break;
        case 5: trans.scale(-1, 1); trans.rotate(90); break;
        case 6: trans.rotate(90); break;
        case 7: trans.scale(1, -1); trans.rotate(90); break;
        case 8: trans.rotate(-90); break;
        default: needsTransform = false; break;
    }

    return needsTransform ? src.transformed(trans, Qt::SmoothTransformation) : src;
}

// --- 缩放：Qt 路径 ---

QImage ImageLib::scaled(QImage source, QSize destSize, ScalingFilter filter) {
    if (source.isNull()) return QImage();

    if (destSize == source.size()) {
        return source; // move elision
    }

    QImage scaleTarget = std::move(source);

    if (scaleTarget.format() == QImage::Format_Indexed8) {
        QImage::Format newFmt = scaleTarget.hasAlphaChannel()
                                ? QImage::Format_ARGB32
                                : QImage::Format_RGB32;
        scaleTarget = std::move(scaleTarget).convertToFormat(newFmt);
    }

#ifdef USE_OPENCV
    if (filter > 1 && !QtOcv::isSupported(scaleTarget.format()))
        filter = QI_FILTER_BILINEAR;
#endif

    auto scaleQtMove = [](QImage img, QSize destSize, bool smooth) -> QImage {
        if (destSize == img.size()) return img;

        if (destSize.width() < img.width() || destSize.height() < img.height()) {
            if (destSize.width() <= destSize.height()) {
                return img.scaledToWidth(destSize.width(),
                    smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
            }
            return img.scaledToHeight(destSize.height(),
                smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
        }

        return img.scaled(destSize, Qt::KeepAspectRatio,
            smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
    };

    switch (filter) {
        case QI_FILTER_NEAREST:
            return scaleQtMove(std::move(scaleTarget), destSize, false);

        case QI_FILTER_BILINEAR:
            return scaleQtMove(std::move(scaleTarget), destSize, true);

#ifdef USE_OPENCV
        case QI_FILTER_CV_BILINEAR_SHARPEN:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_LINEAR, 0);

        case QI_FILTER_CV_CUBIC:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 0);

        case QI_FILTER_CV_CUBIC_SHARPEN:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 1);
#endif

        default:
            return scaleQtMove(std::move(scaleTarget), destSize, true);
    }
}

QImage ImageLib::scaled_Qt(const QImage &source, QSize destSize, bool smooth) {
    if (source.isNull()) return QImage();
    if (destSize == source.size()) {
        return source;  // 源与目标大小相同，直接返回（但 source 是 const&，会拷贝）
    }

    // Qt 6.10优化：根据缩放方向选择最优方法
    if (destSize.width() < source.width() || destSize.height() < source.height()) {
        // 缩小操作 - 使用专门的缩小方法，性能更好
        if (destSize.width() <= destSize.height()) {
            return source.scaledToWidth(destSize.width(),
                                      smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
        }
        return source.scaledToHeight(destSize.height(),
                                   smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
    }
    // 放大操作 - 使用通用scaled方法
    return source.scaled(destSize,
                       Qt::KeepAspectRatio,
                       smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
}

#ifdef USE_OPENCV
QImage ImageLib::scaled_CV(QImage source, QSize destSize,
                           cv::InterpolationFlags filter, int sharpen)
{
    if (source.isNull()) return QImage();
    if (destSize == source.size()) return source;

    // 避免 QImage 隐式 detach
    const QImage& srcRef = source;

    QtOcv::MatColorOrder order;
    cv::Mat srcMat = QtOcv::image2Mat_shared(srcRef, &order);
    if (srcMat.empty()) return QImage();

    cv::InterpolationFlags actualFilter = filter;
    int actualSharpen = sharpen;

    if (destSize.width() < srcRef.width()) {
        float scale = float(destSize.width()) / float(srcRef.width());
        if (scale < 0.5f && filter != cv::INTER_NEAREST) {
            actualFilter = cv::INTER_AREA;
            if (filter == cv::INTER_CUBIC)
                actualSharpen = 1;
        }
    }

    cv::Mat dstMat;
    cv::resize(srcMat, dstMat,
               cv::Size(destSize.width(), destSize.height()),
               0, 0, actualFilter);

    if (actualSharpen && actualFilter != cv::INTER_NEAREST) {
        double amount = 0.25 * actualSharpen;
        cv::Mat blurred;
        cv::GaussianBlur(dstMat, blurred, cv::Size(0, 0), 2);
        cv::addWeighted(dstMat, 1.0 + amount, blurred, -amount, 0, dstMat);
    }

    // 原版调用，正确且高效
    return QtOcv::mat2Image(dstMat, order, srcRef.format());
}
#endif