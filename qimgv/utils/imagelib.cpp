#include "imagelib.h"
#include <memory>

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

// --- 90° 倍数旋转的快速路径 ---

// 90° 倍数旋转像素 1:1 映射：180° 用两次原地翻转实现（零分配），
// 其余用 FastTransformation（避免 Smooth 的插值开销及包围盒取整造成的轻微模糊）
static QImage rotated90Multiple(QImage src, int grad) {
    const int quarter = ((grad / 90) % 4 + 4) % 4;   // 归一化到 0..3
    switch (quarter) {
        case 0:
            return std::move(src);
        case 2: // 180°：水平 + 垂直原地翻转
            return std::move(src).flipped(Qt::Horizontal).flipped(Qt::Vertical);
        default: {
            // 顺时针 90°（quarter==1）或逆时针 90°（quarter==3）
            QTransform trans;
            trans.rotate(quarter == 1 ? 90 : -90);
            return src.transformed(trans, Qt::FastTransformation);
        }
    }
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
    // 90° 倍数旋转走快速路径（180° 原地翻转零拷贝）
    if (grad % 90 == 0) {
        return rotated90Multiple(std::move(src), grad);
    }
    // 任意角度才用平滑变换
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

    switch (orientation) {
        // 纯镜像/180°：Qt6 flipped() 右值重载原地翻转，零拷贝零分配
        case 2: return std::move(src).flipped(Qt::Horizontal);
        case 3: return std::move(src).flipped(Qt::Horizontal).flipped(Qt::Vertical);
        case 4: return std::move(src).flipped(Qt::Vertical);
        // 90°/270°：复用 90° 快速路径
        case 6: return rotated90Multiple(std::move(src), 90);
        case 8: return rotated90Multiple(std::move(src), -90);
        // 90° + 镜像：像素 1:1 映射，FastTransformation 避免插值及包围盒取整模糊
        case 5: {
            QTransform trans;
            trans.scale(-1, 1); trans.rotate(90);
            return src.transformed(trans, Qt::FastTransformation);
        }
        case 7: {
            QTransform trans;
            trans.scale(1, -1); trans.rotate(90);
            return src.transformed(trans, Qt::FastTransformation);
        }
        default: return std::move(src);
    }
}

// --- 缩放：统一使用 Qt 平滑变换 ---

QImage ImageLib::scaled(QImage source, QSize destSize) {
    if (source.isNull()) return QImage();

    if (destSize == source.size()) {
        return std::move(source);   // ✅ 消除不必要的深拷贝
    }

    // 直接交给 Qt 的平滑变换：Qt 6.10.1 已能处理 Format_Indexed8，无需提前转换
    return source.scaled(destSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}