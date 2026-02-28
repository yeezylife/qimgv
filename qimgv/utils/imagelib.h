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
    // 旋转变换：返回对象而非指针
    static QImage rotatedRaw(const QImage &src, int grad);
    static QImage rotated(std::shared_ptr<const QImage> src, int grad);

    // 裁剪
    static QImage croppedRaw(const QImage &src, QRect newRect);
    static QImage cropped(std::shared_ptr<const QImage> src, QRect newRect);

    // 翻转
    static QImage flippedHRaw(const QImage &src);
    static QImage flippedH(std::shared_ptr<const QImage> src);
    static QImage flippedVRaw(const QImage &src);
    static QImage flippedV(std::shared_ptr<const QImage> src);

    // 缩放
    static QImage scaled(std::shared_ptr<const QImage> source, QSize destSize, ScalingFilter filter);
    static QImage scaled_Qt(std::shared_ptr<const QImage> source, QSize destSize, bool smooth);

#ifdef USE_OPENCV
    static QImage scaled_CV(std::shared_ptr<const QImage> source, QSize destSize, cv::InterpolationFlags filter, int sharpen);
#endif

    // EXIF 处理：保持 unique_ptr 或改为 QImage 均可，这里为了统一也建议逐步改为 QImage
    static std::unique_ptr<const QImage> exifRotated(std::unique_ptr<const QImage> src, int orientation);
    static std::unique_ptr<QImage> exifRotated(std::unique_ptr<QImage> src, int orientation);

    static void recolor(QPixmap &pixmap, const QColor &color);
};