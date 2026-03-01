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

// 版本一的 EXIF 旋转逻辑 - 值传递，利用 Qt 隐式共享
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

// 模板辅助函数 - 使用 Qt 隐式共享，无需 new QImage
template<typename T>
static T processExif(T src, int orientation) {
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
        // QImage 使用隐式共享，copy() 会创建写时复制的副本
        // 避免了 new QImage 的手动内存管理
        return src->transformed(trans, Qt::SmoothTransformation);
    }
    
    return src;
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
        // 利用 Qt 隐式共享创建副本
        std::unique_ptr<const QImage> result(new QImage(src->transformed(trans, Qt::SmoothTransformation)));
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
        // 利用 Qt 隐式共享创建副本
        std::unique_ptr<QImage> result(new QImage(src->transformed(trans, Qt::SmoothTransformation)));
        return result;
    }
    
    return src;
}

QImage ImageLib::scaled(std::shared_ptr<const QImage> source, QSize destSize, ScalingFilter filter) {
    if (!source || source->isNull()) return QImage();
    
    auto scaleTarget = source;
    if (source->format() == QImage::Format_Indexed8) {
        QImage::Format newFmt = source->hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
        scaleTarget = std::make_shared<QImage>(source->convertToFormat(newFmt));
    }
    
#ifdef USE_OPENCV
    if (filter > 1 && !QtOcv::isSupported(scaleTarget->format())) filter = QI_FILTER_BILINEAR;
#endif
    
    switch (filter) {
        case QI_FILTER_NEAREST:
            return scaled_Qt(scaleTarget, destSize, false);
        case QI_FILTER_BILINEAR:
            return scaled_Qt(scaleTarget, destSize, true);
#ifdef USE_OPENCV
        case QI_FILTER_CV_BILINEAR_SHARPEN:
            return scaled_CV(scaleTarget, destSize, cv::INTER_LINEAR, 0);
        case QI_FILTER_CV_CUBIC:
            return scaled_CV(scaleTarget, destSize, cv::INTER_CUBIC, 0);
        case QI_FILTER_CV_CUBIC_SHARPEN:
            return scaled_CV(scaleTarget, destSize, cv::INTER_CUBIC, 1);
#endif
        default:
            return scaled_Qt(scaleTarget, destSize, true);
    }
}

QImage ImageLib::scaled_Qt(std::shared_ptr<const QImage> source, QSize destSize, bool smooth) {
    if (!source || source->isNull()) return QImage();
    return source->scaled(destSize, Qt::IgnoreAspectRatio, smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
}

#ifdef USE_OPENCV
QImage ImageLib::scaled_CV(std::shared_ptr<const QImage> source, QSize destSize, cv::InterpolationFlags filter, int sharpen) {
    if (!source || source->isNull()) return QImage();
    
    // 尺寸相同时直接返回 - 利用 Qt 隐式共享，无需复制
    if (destSize == source->size()) return *source;
    
    QtOcv::MatColorOrder order;
    cv::Mat srcMat = QtOcv::image2Mat_shared(*source, &order);
    
    // 版本一的优化逻辑：缩小超过 50% 时使用 INTER_AREA
    cv::InterpolationFlags actualFilter = filter;
    int actualSharpen = sharpen;
    
    if (destSize.width() < source->width()) {
        float scale = static_cast<float>(destSize.width()) / source->width();
        
        if (scale < 0.5f && filter != cv::INTER_NEAREST) {
            // 缩小超过 50% 时，自动切换到更优质的 INTER_AREA
            actualFilter = cv::INTER_AREA;
            
            // 如果原本使用 CUBIC，则启用锐化补偿
            if (filter == cv::INTER_CUBIC) {
                actualSharpen = 1;
            }
        }
    }
    
    cv::Mat dstMat;
    cv::resize(srcMat, dstMat, cv::Size(destSize.width(), destSize.height()), 0, 0, actualFilter);
    
    // 锐化处理
    if (actualSharpen && actualFilter != cv::INTER_NEAREST) {
        double amount = 0.25 * actualSharpen;
        cv::Mat blurred;
        cv::GaussianBlur(dstMat, blurred, cv::Size(0, 0), 2);
        cv::addWeighted(dstMat, 1.0 + amount, blurred, -amount, 0, dstMat);
    }
    
    return QtOcv::mat2Image(dstMat, order, source->format());
}
#endif