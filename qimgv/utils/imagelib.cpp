#include "imagelib.h"
#include <memory>

#ifdef USE_OPENCV
#include "3rdparty/QtOpenCV/cvmatandqimage.h"
#endif

// ----------------- 基础工具 -----------------

QImage ImageLib::applyTransform(QImage src, const QTransform &t) {
    if (src.isNull()) return QImage();
    if (t.isIdentity()) return src;
    return src.transformed(t, Qt::SmoothTransformation);
}

bool ImageLib::buildExifTransform(int orientation, QTransform &t) {
    switch (orientation) {
        case 2: t.scale(-1, 1); break;
        case 3: t.rotate(180); break;
        case 4: t.scale(1, -1); break;
        case 5: t.scale(-1, 1); t.rotate(90); break;
        case 6: t.rotate(90); break;
        case 7: t.scale(1, -1); t.rotate(90); break;
        case 8: t.rotate(-90); break;
        default: return false;
    }
    return true;
}

// ----------------- recolor -----------------

void ImageLib::recolor(QPixmap &pixmap, const QColor &color) {
    if (pixmap.isNull()) return;

    QPainter p(&pixmap);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pixmap.rect(), color);
}

// ----------------- rotate -----------------

QImage ImageLib::rotatedRaw(const QImage &src, int grad) {
    if (src.isNull()) return QImage();
    QTransform t;
    t.rotate(grad);
    return applyTransform(QImage(src), t);
}

QImage ImageLib::rotated(const QImage &src, int grad) {
    return rotatedRaw(src, grad);
}

// ----------------- crop -----------------

QImage ImageLib::croppedRaw(const QImage &src, QRect newRect) {
    if (!src.isNull() && src.rect().contains(newRect)) {
        return src.copy(newRect);
    }
    return QImage();
}

QImage ImageLib::cropped(const QImage &src, QRect newRect) {
    if (src.isNull()) return QImage();
    return croppedRaw(src, newRect);
}

// ----------------- flip -----------------

QImage ImageLib::flippedHRaw(QImage src) {
    if (src.isNull()) return QImage();
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

// ----------------- EXIF -----------------

QImage ImageLib::exifRotated(QImage src, int orientation) {
    if (src.isNull() || orientation <= 1) return src;

    QTransform t;
    if (!buildExifTransform(orientation, t)) return src;

    return applyTransform(std::move(src), t);
}

std::unique_ptr<const QImage> ImageLib::exifRotated(std::unique_ptr<const QImage> src, int orientation) {
    if (!src || src->isNull() || orientation <= 1) return src;

    QTransform t;
    if (!buildExifTransform(orientation, t)) return src;

    return std::make_unique<const QImage>(
        applyTransform(QImage(*src), t)
    );
}

std::unique_ptr<QImage> ImageLib::exifRotated(std::unique_ptr<QImage> src, int orientation) {
    if (!src || src->isNull() || orientation <= 1) return src;

    QTransform t;
    if (!buildExifTransform(orientation, t)) return src;

    return std::make_unique<QImage>(
        applyTransform(std::move(*src), t)
    );
}

// ----------------- scale -----------------

QImage ImageLib::scaled(QImage source, QSize destSize, ScalingFilter filter) {
    if (source.isNull()) return QImage();

    QImage scaleTarget = std::move(source);

    // 只保留真正有收益的转换
    if (scaleTarget.format() == QImage::Format_Indexed8) {
        QImage::Format newFmt = scaleTarget.hasAlphaChannel()
                                ? QImage::Format_ARGB32
                                : QImage::Format_RGB32;
        scaleTarget = scaleTarget.convertToFormat(newFmt);
    }

#ifdef USE_OPENCV
    if (filter > 1 && !QtOcv::isSupported(scaleTarget.format()))
        filter = QI_FILTER_BILINEAR;
#endif

    switch (filter) {
        case QI_FILTER_NEAREST:
            return scaled_Qt(scaleTarget, destSize, false);

        case QI_FILTER_BILINEAR:
            return scaled_Qt(scaleTarget, destSize, true);

#ifdef USE_OPENCV
        case QI_FILTER_CV_BILINEAR_SHARPEN:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_LINEAR, 0);

        case QI_FILTER_CV_CUBIC:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 0);

        case QI_FILTER_CV_CUBIC_SHARPEN:
            return scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 1);
#endif

        default:
            return scaled_Qt(scaleTarget, destSize, true);
    }
}

// ----------------- Qt scale -----------------

QImage ImageLib::scaled_Qt(const QImage &source, QSize destSize, bool smooth) {
    if (source.isNull()) return QImage();

    const auto mode = smooth ? Qt::SmoothTransformation : Qt::FastTransformation;

    // ✅ 修正：必须双向缩小
    if (destSize.width() <= source.width() &&
        destSize.height() <= source.height()) {

        if (destSize.width() <= destSize.height()) {
            return source.scaledToWidth(destSize.width(), mode);
        }
        return source.scaledToHeight(destSize.height(), mode);
    }

    return source.scaled(destSize, Qt::KeepAspectRatio, mode);
}

#ifdef USE_OPENCV
QImage ImageLib::scaled_CV(QImage source, QSize destSize,
                           cv::InterpolationFlags filter, int sharpen) {
    if (source.isNull()) return QImage();
    if (destSize == source.size()) return source;

    QtOcv::MatColorOrder order;
    cv::Mat srcMat = QtOcv::image2Mat_shared(source, &order);

    cv::InterpolationFlags actualFilter = filter;
    int actualSharpen = sharpen;

    if (destSize.width() < source.width()) {
        float scale = static_cast<float>(destSize.width()) /
                      static_cast<float>(source.width());
        if (scale < 0.5f && filter != cv::INTER_NEAREST) {
            actualFilter = cv::INTER_AREA;
            if (filter == cv::INTER_CUBIC) {
                actualSharpen = 1;
            }
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

    return QtOcv::mat2Image(dstMat, order, source.format());
}
#endif