#pragma once

#include <QImage>
#include <QPainter>
#include <QColor>
#include <QRect>
#include <QSize>

#include "settings.h"

#ifdef USE_OPENCV
#include <opencv2/imgproc.hpp>
#endif

class ImageLib {
public:
    // ============================
    // 旋转
    // ============================
    static QImage rotatedRaw(const QImage &src, int grad);

    // 按值传递：允许调用方 std::move，避免不必要拷贝
    static QImage rotated(QImage src, int grad);

    // ============================
    // 裁剪
    // ============================
    static QImage croppedRaw(const QImage &src, QRect newRect);

    // 按值：支持 move 优化
    static QImage cropped(QImage src, QRect newRect);

    // ============================
    // 翻转（天然适合按值）
    // ============================
    static QImage flippedHRaw(QImage src);
    static QImage flippedH(QImage src);

    static QImage flippedVRaw(QImage src);
    static QImage flippedV(QImage src);

    // ============================
    // 缩放（主入口）
    // ============================
    // ⭐ 按值传递：允许 move + 避免 detach
    static QImage scaled(QImage source, QSize destSize, ScalingFilter filter);

    // Qt fallback（小图 / 无 vips）
    static QImage scaled_Qt(const QImage &source, QSize destSize, bool smooth);

#ifdef USE_OPENCV
    // OpenCV 路径（可选）
    static QImage scaled_CV(QImage source,
                           QSize destSize,
                           cv::InterpolationFlags filter,
                           int sharpen);
#endif

    // ============================
    // EXIF 方向修正
    // ============================
    static QImage exifRotated(QImage src, int orientation);

    // ============================
    // Pixmap 重着色（UI 用）
    // ============================
    static void recolor(QPixmap &pixmap, const QColor &color);
};