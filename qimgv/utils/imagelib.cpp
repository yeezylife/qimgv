#include "imagelib.h"

#ifdef USE_OPENCV
#include "3rdparty/QtOpenCV/cvmatandqimage.h"
#endif

void ImageLib::recolor(QPixmap &pixmap, const QColor &color) {
    if (pixmap.isNull()) return;
    QPainter p(&pixmap);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pixmap.rect(), color);
}

QImage ImageLib::rotatedRaw(const QImage &src, int grad) {
    if (src.isNull()) return QImage();
    QTransform transform;
    transform.rotate(grad);
    return src.transformed(transform, Qt::SmoothTransformation);
}

QImage ImageLib::rotated(QImage src, int grad) {
    if (src.isNull()) return QImage();
    // 这里 rotatedRaw 接 const QImage&，不涉及 && 优化
    return rotatedRaw(src, grad);
}

QImage ImageLib::croppedRaw(const QImage &src, QRect newRect) {
    if (!src.isNull() && src.rect().contains(newRect)) {
        return src.copy(newRect);
    }
    return QImage();
}

QImage ImageLib::cropped(QImage src, QRect newRect) {
    if (src.isNull()) return QImage();
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

// unique_ptr 版本 - const QImage
std::unique_ptr<const QImage> ImageLib::exifRotated(std::unique_ptr<const QImage> src, int orientation) {
    if (!src || src->isNull() || orientation <= 1) return src;

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

    if (needsTransform) {
        std::unique_ptr<const QImage> result(
            new QImage(src->transformed(trans, Qt::SmoothTransformation))
        );
        return result;
    }

    return src;
}

// unique_ptr 版本 - 非 const QImage
std::unique_ptr<QImage> ImageLib::exifRotated(std::unique_ptr<QImage> src, int orientation) {
    if (!src || src->isNull() || orientation <= 1) return src;

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

    if (needsTransform) {
        std::unique_ptr<QImage> result(
            new QImage(src->transformed(trans, Qt::SmoothTransformation))
        );
        return result;
    }

    return src;
}

// --- 缩放：Qt 路径 ---

QImage ImageLib::scaled(QImage source, QSize destSize, ScalingFilter filter) {
    if (source.isNull()) return QImage();

    QImage scaleTarget = std::move(source);
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

    QImage result;
    switch (filter) {
        case QI_FILTER_NEAREST: {
            QImage tmp = scaled_Qt(std::move(scaleTarget), destSize, false);
            if (!tmp.isNull()) result = std::move(tmp);
            break;
        }
        case QI_FILTER_BILINEAR: {
            QImage tmp = scaled_Qt(std::move(scaleTarget), destSize, true);
            if (!tmp.isNull()) result = std::move(tmp);
            break;
        }
#ifdef USE_OPENCV
        case QI_FILTER_CV_BILINEAR_SHARPEN: {
            QImage tmp = scaled_CV(std::move(scaleTarget), destSize, cv::INTER_LINEAR, 0);
            if (!tmp.isNull()) result = std::move(tmp);
            break;
        }
        case QI_FILTER_CV_CUBIC: {
            QImage tmp = scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 0);
            if (!tmp.isNull()) result = std::move(tmp);
            break;
        }
        case QI_FILTER_CV_CUBIC_SHARPEN: {
            QImage tmp = scaled_CV(std::move(scaleTarget), destSize, cv::INTER_CUBIC, 1);
            if (!tmp.isNull()) result = std::move(tmp);
            break;
        }
#endif
        default: {
            QImage tmp = scaled_Qt(std::move(scaleTarget), destSize, true);
            if (!tmp.isNull()) result = std::move(tmp);
            break;
        }
    }

    return result;
}

QImage ImageLib::scaled_Qt(QImage source, QSize destSize, bool smooth) {
    if (source.isNull()) return QImage();
    return source.scaled(destSize,
                         Qt::IgnoreAspectRatio,
                         smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
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
        float scale = static_cast<float>(destSize.width()) / source.width();
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

    QImage out = QtOcv::mat2Image(dstMat, order, source.format());
    return out;
}
#endif
