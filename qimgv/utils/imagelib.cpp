#include "imagelib.h"
#include <memory>

#include <QPainter>
#include <QTransform>
#include <QThread>

#ifdef USE_VIPS
#include <vips/vips8>
#include <mutex>
using namespace vips;

// ============================
// libvips 初始化（线程安全）
// ============================
static void initVipsOnce()
{
    static std::once_flag flag;
    std::call_once(flag, [] {
        if (VIPS_INIT("qimgv")) {
            qFatal("Failed to init libvips");
        }
        // 使用系统线程数，避免和 Qt 线程池冲突
        vips_concurrency(QThread::idealThreadCount());
    });
}

// ============================
// QImage -> VImage（安全版）
// 最多 1 次拷贝（格式不匹配时）
// ============================
static VImage vipsFromQImage(const QImage &img)
{
    QImage src = img;

    // ⭐ 统一为 RGBA8888（与 vips 完全匹配）
    if (src.format() != QImage::Format_RGBA8888) {
        src = src.convertToFormat(QImage::Format_RGBA8888);
    }

    return VImage::new_from_memory_copy(
        src.constBits(),
        static_cast<size_t>(src.width() * src.height() * 4),
        src.width(),
        src.height(),
        4,
        VIPS_FORMAT_UCHAR
    );
}

// ============================
// VImage -> QImage（安全版）
// 1 次拷贝（不可避免）
// ============================
static QImage qimageFromVips(const VImage &img)
{
    VImage out = img;

    if (out.format() != VIPS_FORMAT_UCHAR) {
        out = out.cast(VIPS_FORMAT_UCHAR);
    }

    int bands = out.bands();

    // 灰度 → RGB
    if (bands == 1) {
        out = out.bandjoin(out).bandjoin(out);
        bands = 3;
    }

    // RGB → RGBA
    if (bands == 3) {
        VImage alpha = VImage::black(out.width(), out.height())
                           .new_from_image(255);
        out = out.bandjoin(alpha);
        bands = 4;
    }

    // ⭐ 强制连续内存（必要）
    out = out.copy_memory();

    size_t size = 0;
    void *data = out.write_to_memory(&size);

    if (!data) {
        return QImage();
    }

    // QImage 接管内存
    QImage result(
        static_cast<uchar *>(data),
        out.width(),
        out.height(),
        out.width() * 4,
        QImage::Format_RGBA8888,
        [](void *p) { g_free(p); },
        data
    );

    return result;
}

// ============================
// 高质量缩放（libvips）
// ============================
static QImage scaled_vips(QImage source, QSize destSize)
{
    if (source.isNull() || destSize.isEmpty())
        return QImage();

    if (source.size() == destSize)
        return source;

    const int sw = source.width();
    const int sh = source.height();

    // ⭐ 小图走 Qt（更快）
    if (sw * sh < 512 * 512) {
        return source.scaled(destSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    initVipsOnce();

    double scale = std::min(
        static_cast<double>(destSize.width()) / sw,
        static_cast<double>(destSize.height()) / sh
    );

    if (scale <= 0.0)
        return QImage();

    // ⭐ 避免无意义处理
    if (std::abs(scale - 1.0) < 1e-6)
        return source;

    VImage in  = vipsFromQImage(source);

    // ⭐ libvips 自动选择：
    // downscale → area
    // upscale   → lanczos3
    VImage out = in.resize(scale);

    return qimageFromVips(out);
}

#endif // USE_VIPS

// ============================
// 颜色重绘
// ============================
void ImageLib::recolor(QPixmap &pixmap, const QColor &color) {
    if (pixmap.isNull()) return;
    QPainter p(&pixmap);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pixmap.rect(), color);
}

// ============================
// 旋转
// ============================
QImage ImageLib::rotatedRaw(const QImage &src, int grad) {
    if (src.isNull()) return QImage();
    QTransform transform;
    transform.rotate(grad);
    return src.transformed(transform, Qt::SmoothTransformation);
}

QImage ImageLib::rotated(const QImage &src, int grad) {
    if (grad % 360 == 0) {
        return src;
    }
    return rotatedRaw(src, grad);
}

// ============================
// 裁剪
// ============================
QImage ImageLib::croppedRaw(const QImage &src, QRect newRect) {
    if (!src.isNull() && src.rect().contains(newRect)) {
        return src.copy(newRect);
    }
    return QImage();
}

QImage ImageLib::cropped(const QImage &src, QRect newRect) {
    if (src.rect() == newRect) {
        return src;
    }
    return croppedRaw(src, newRect);
}

// ============================
// 翻转
// ============================
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

// ============================
// EXIF 旋转
// ============================
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

// ============================
// 缩放接口（统一入口）
// ============================
QImage ImageLib::scaled(QImage src, QSize destSize, ScalingFilter)
{
#ifdef USE_VIPS
    return scaled_vips(std::move(src), destSize);
#else
    return src.scaled(destSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
#endif
}