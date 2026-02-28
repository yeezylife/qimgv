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

QImage ImageLib::rotated(std::shared_ptr<const QImage> src, int grad) {
    return src ? rotatedRaw(*src, grad) : QImage();
}

QImage ImageLib::croppedRaw(const QImage &src, QRect newRect) {
    if (!src.isNull() && src.rect().contains(newRect)) {
        return src.copy(newRect);
    }
    return QImage();
}

QImage ImageLib::cropped(std::shared_ptr<const QImage> src, QRect newRect) {
    return src ? croppedRaw(*src, newRect) : QImage();
}

QImage ImageLib::flippedHRaw(const QImage &src) {
    return src.mirrored(true, false);
}

QImage ImageLib::flippedH(std::shared_ptr<const QImage> src) {
    return src ? flippedHRaw(*src) : QImage();
}

QImage ImageLib::flippedVRaw(const QImage &src) {
    return src.mirrored(false, true);
}

QImage ImageLib::flippedV(std::shared_ptr<const QImage> src) {
    return src ? flippedVRaw(*src) : QImage();
}

// EXIF 内部辅助逻辑
template <typename T>
static std::unique_ptr<T> processExif(std::unique_ptr<T> src, int orientation) {
    if (!src || src->isNull() || orientation <= 0) return src;
    QTransform trans;
    bool needsTransform = true;
    switch(orientation) {
        case 1: trans.scale(-1, 1); break;
        case 2: trans.scale(1, -1); break;
        case 3: trans.rotate(180); break;
        case 4: trans.rotate(90); break;
        case 5: trans.scale(-1, 1); trans.rotate(90); break; 
        case 6: trans.scale(1, -1); trans.rotate(90); break;
        case 7: trans.rotate(-90); break;
        default: needsTransform = false; break;
    }
    if (needsTransform) {
        src.reset(new QImage(src->transformed(trans, Qt::SmoothTransformation)));
    }
    return src;
}

std::unique_ptr<const QImage> ImageLib::exifRotated(std::unique_ptr<const QImage> src, int orientation) {
    return processExif(std::move(src), orientation);
}

std::unique_ptr<QImage> ImageLib::exifRotated(std::unique_ptr<QImage> src, int orientation) {
    return processExif(std::move(src), orientation);
}

QImage ImageLib::scaled(std::shared_ptr<const QImage> source, QSize destSize, ScalingFilter filter) {
    if (!source || source->isNull()) return QImage();

    auto scaleTarget = source;
    if (source->format() == QImage::Format_Indexed8) {
        QImage::Format newFmt = source->hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
        scaleTarget = std::make_shared<QImage>(source->convertToFormat(newFmt));
    }

#ifdef USE_OPENCV
    if (filter > 1 && !QtOcv::isSupported(scaleTarget->format()))
        filter = QI_FILTER_BILINEAR;
#endif

    switch (filter) {
        case QI_FILTER_NEAREST:  return scaled_Qt(scaleTarget, destSize, false);
        case QI_FILTER_BILINEAR: return scaled_Qt(scaleTarget, destSize, true);
#ifdef USE_OPENCV
        case QI_FILTER_CV_BILINEAR_SHARPEN: return scaled_CV(scaleTarget, destSize, cv::INTER_LINEAR, 0);
        case QI_FILTER_CV_CUBIC:           return scaled_CV(scaleTarget, destSize, cv::INTER_CUBIC, 0);
        case QI_FILTER_CV_CUBIC_SHARPEN:   return scaled_CV(scaleTarget, destSize, cv::INTER_CUBIC, 1);
#endif
        default: return scaled_Qt(scaleTarget, destSize, true);
    }
}

QImage ImageLib::scaled_Qt(std::shared_ptr<const QImage> source, QSize destSize, bool smooth) {
    if (!source || source->isNull()) return QImage();
    return source->scaled(destSize, Qt::IgnoreAspectRatio, 
                          smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
}

#ifdef USE_OPENCV
QImage ImageLib::scaled_CV(std::shared_ptr<const QImage> source, QSize destSize, cv::InterpolationFlags filter, int sharpen) {
    if (!source || source->isNull()) return QImage();
    
    QtOcv::MatColorOrder order;
    cv::Mat srcMat = QtOcv::image2Mat_shared(*source, &order);
    cv::Size destSizeCv(destSize.width(), destSize.height());

    if (destSize == source->size()) return *source;

    cv::Mat dstMat;
    if (destSize.width() < source->width()) {
        float scale = (float)destSize.width() / source->width();
        if (scale < 0.5f && filter != cv::INTER_NEAREST) {
            if (filter == cv::INTER_CUBIC) sharpen = 1;
            filter = cv::INTER_AREA;
        }
    }

    cv::resize(srcMat, dstMat, destSizeCv, 0, 0, filter);

    if (sharpen && filter != cv::INTER_NEAREST) {
        double amount = 0.25 * sharpen;
        cv::Mat blurred;
        cv::GaussianBlur(dstMat, blurred, cv::Size(0, 0), 2);
        cv::addWeighted(dstMat, 1.0 + amount, blurred, -amount, 0, dstMat);
    }

    return QtOcv::mat2Image(dstMat, order, source->format());
}
#endif