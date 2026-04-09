#pragma once
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QRect>
#include <QSize>
#include <memory>
#include "settings.h"

#ifdef USE_OPENCV
#include <opencv2/imgproc.hpp>
#endif

class ImageLib {
public:
    static QImage rotatedRaw(const QImage &src, int grad);
    // Take by-value so callers can move images in to avoid copies (Qt6 move-friendly)
    static QImage rotated(QImage src, int grad);

    static QImage croppedRaw(const QImage &src, QRect newRect);
    // By-value to enable move optimization
    static QImage cropped(QImage src, QRect newRect);

    // Accept by-value for operations that produce a new image; callers may move
    static QImage flippedHRaw(QImage src);
    static QImage flippedH(QImage src);
    static QImage flippedVRaw(QImage src);
    static QImage flippedV(QImage src);

    // Scale helpers accept by-value to allow move-optimization
    static QImage scaled(QImage source, QSize destSize, ScalingFilter filter);

#ifdef USE_OPENCV
    static QImage scaled_CV(QImage source, QSize destSize, cv::InterpolationFlags filter, int sharpen);
#endif

    // EXIF 处理：也改为返回 QImage 以保持一致性
    static QImage exifRotated(QImage src, int orientation);

    static void recolor(QPixmap &pixmap, const QColor &color);
};