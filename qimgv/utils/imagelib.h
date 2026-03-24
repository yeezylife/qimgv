#pragma once
#include <QImage>
#include <QPainter>
#include <QColor>
#include <QRect>
#include <QSize>
#include <QTransform>
#include <memory>
#include "settings.h"

#ifdef USE_OPENCV
#include <opencv2/imgproc.hpp>
#endif

class ImageLib {
public:
    static QImage rotatedRaw(const QImage &src, int grad);
    static QImage rotated(const QImage &src, int grad);

    static QImage croppedRaw(const QImage &src, QRect newRect);
    static QImage cropped(const QImage &src, QRect newRect);

    static QImage flippedHRaw(QImage src);
    static QImage flippedH(QImage src);
    static QImage flippedVRaw(QImage src);
    static QImage flippedV(QImage src);

    static QImage scaled(QImage source, QSize destSize, ScalingFilter filter);
    static QImage scaled_Qt(const QImage &source, QSize destSize, bool smooth);

#ifdef USE_OPENCV
    static QImage scaled_CV(QImage source, QSize destSize,
                           cv::InterpolationFlags filter, int sharpen);
#endif

    static QImage exifRotated(QImage src, int orientation);

    static std::unique_ptr<const QImage> exifRotated(std::unique_ptr<const QImage> src, int orientation);
    static std::unique_ptr<QImage> exifRotated(std::unique_ptr<QImage> src, int orientation);

    static void recolor(QPixmap &pixmap, const QColor &color);

private:
    // 统一变换入口（核心优化点）
    static QImage applyTransform(QImage src, const QTransform &t);

    // EXIF transform 构建
    static bool buildExifTransform(int orientation, QTransform &t);
};