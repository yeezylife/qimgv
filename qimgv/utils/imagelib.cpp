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

QImage ImageLib::rotated(QImage src, int grad) {
    // 旋转角度为 360° 的整数倍，无需变换，直接返回源图像
    if (grad % 360 == 0) {
        return std::move(src);
    }
    return rotatedRaw(src, grad);
}

QImage ImageLib::croppedRaw(const QImage &src, QRect newRect) {
    if (!src.isNull() && src.rect().contains(newRect)) {
        return src.copy(newRect);
    }
    return QImage();
}

QImage ImageLib::cropped(QImage src, QRect newRect) {
    // 裁剪区域等于原图大小，直接返回源图像
    if (src.rect() == newRect) {
        return std::move(src);
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
    return flippedHRaw(std::move(src));
}

QImage ImageLib::flippedVRaw(QImage src) {
    if (src.isNull()) return QImage();
    return std::move(src).flipped(Qt::Vertical);
}

QImage ImageLib::flippedV(QImage src) {
    return flippedVRaw(std::move(src));
}

// --- EXIF 旋转：transformed 没有 && 版本，只能深拷贝 ---

QImage ImageLib::exifRotated(QImage src, int orientation) {
    if (src.isNull() || orientation <= 1) return std::move(src);

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

    return needsTransform ? src.transformed(trans, Qt::SmoothTransformation) : std::move(src);
}

// --- 缩放：Qt 路径 ---

QImage ImageLib::scaled(QImage source, QSize destSize, ScalingFilter filter) {
    if (source.isNull()) return QImage();

    if (destSize == source.size()) {
        return std::move(source);   // ✅ 消除不必要的深拷贝
    }

    QImage scaleTarget = std::move(source);

    if (scaleTarget.format() == QImage::Format_Indexed8) {
        scaleTarget = std::move(scaleTarget)
                        .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

#ifdef USE_OPENCV
    if (filter > 1 && !QtOcv::isSupported(scaleTarget.format()))
        filter = QI_FILTER_BILINEAR;
#endif

    // lambda 使用 const 引用，避免不必要的拷贝（scaled 不支持右值重载）
    auto scaleQtMove = [](const QImage& img, QSize destSize, bool smooth) -> QImage {
        Qt::TransformationMode mode = smooth ? Qt::SmoothTransformation : Qt::FastTransformation;
        return img.scaled(destSize, Qt::KeepAspectRatio, mode);
    };

    switch (filter) {
        case QI_FILTER_NEAREST:
            return scaleQtMove(scaleTarget, destSize, false);

        case QI_FILTER_BILINEAR:
            return scaleQtMove(scaleTarget, destSize, true);

#ifdef USE_OPENCV
        case QI_FILTER_CV_BILINEAR_SHARPEN:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_LINEAR, 0);

        case QI_FILTER_CV_CUBIC:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 0);

        case QI_FILTER_CV_CUBIC_SHARPEN:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 1);
#endif

        default:
            return scaleQtMove(scaleTarget, destSize, true);
    }
}

#ifdef USE_OPENCV
QImage ImageLib::scaled_CV(QImage source, QSize destSize,
                           cv::InterpolationFlags filter, int sharpen)
{
    if (source.isNull()) return QImage();
    if (destSize == source.size()) {
        return std::move(source);   // ✅ 消除不必要的深拷贝
    }

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

    // 零拷贝返回：cv::Mat 内部引用计数自动管理生命周期，安全共享
    return QtOcv::mat2Image_shared(dstMat, srcRef.format());
}
#endif