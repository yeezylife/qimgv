#include "imagestatic.h"
#include "settings.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDebug>
#include <QImageReader>
#include <QImageWriter>

ImageStatic::ImageStatic(std::unique_ptr<DocumentInfo> info, int allocationLimitMB)
    : Image(std::move(info))
{
    loadWith(allocationLimitMB);
}

void ImageStatic::loadWith(int allocationLimitMB) {
    if(isLoaded()) {
        return;
    }

    const auto mimeType = mDocInfo->mimeType().name();
    if(mimeType == "image/vnd.microsoft.icon") {
        loadICO();
    } else {
        loadGeneric(allocationLimitMB);
    }
}

// 将 QImageIOHandler 的变换标志转换为 EXIF orientation 数字
static int transformationToExifOrientation(QImageIOHandler::Transformations t) {
    if (t == QImageIOHandler::TransformationNone) return 1;
    if (t == QImageIOHandler::TransformationRotate180) return 3;
    if (t == QImageIOHandler::TransformationRotate90) return 6;
    if (t == QImageIOHandler::TransformationRotate270) return 8;
    if (t == QImageIOHandler::TransformationMirror) return 2;
    if (t == QImageIOHandler::TransformationFlip) return 4;
    if (t == (QImageIOHandler::TransformationMirror | QImageIOHandler::TransformationRotate270)) return 5;
    if (t == (QImageIOHandler::TransformationMirror | QImageIOHandler::TransformationRotate90)) return 7;
    return 1;
}

void ImageStatic::loadGeneric(int allocationLimitMB) {
    QImageReader reader(mPath);

    reader.setAllocationLimit(allocationLimitMB);

    // 禁用 Qt 自动方向处理，统一由我们自己控制
    reader.setAutoTransform(false);

    QImage imageData;
    if(!reader.read(&imageData)) {
        // reader 失败但 imageData 可能仍然有效（Qt 的特性）
        if(imageData.isNull()) {
            return;
        }
    }

    // 缓存文本元数据（EXIF），保存时避免再次打开原文件
    for(const QString &key : reader.textKeys()) {
        const QString value = reader.text(key);
        if(!value.isEmpty()) {
            mTextMetadata.insert(key, value);
        }
    }

    QImage image = std::move(imageData);

    // ✅ 修复：只在合法 EXIF 范围内处理
    // 复用同一个 reader 读取方向，避免再次打开文件
    const int orientation = transformationToExifOrientation(reader.transformation());
    if (orientation >= 2 && orientation <= 8) {
        image = ImageLib::exifRotated(std::move(image), orientation);
        if (image.isNull()) {
            return;
        }
    }

    // Format_Mono 转换
    if (image.format() == QImage::Format_Mono) {
        image = std::move(image).convertToFormat(QImage::Format_Grayscale8);
    }

    if (image.isNull()) {
        return;
    }

    this->image = std::make_shared<const QImage>(std::move(image));
    mLoaded = true;
    // ⭐ 同步喂给 DocumentInfo（含最终显示尺寸），面板 getExifTags 直接复用，不再二次 QImageReader
    if(mDocInfo)
        mDocInfo->setCachedTextMetadata(mTextMetadata, this->image->size());
}

void ImageStatic::loadICO() {
    // ICO 加载逻辑：只解码面积最大的一帧，避免 QIcon 把所有尺寸都解出来
    QImageReader reader(mPath);
    const int count = reader.imageCount();
    if(count <= 0) {
        qWarning() << "ImageStatic::loadICO() - No images available in ICO file:" << mPath;
        return;
    }

    // 只读尺寸不解码，按面积选最大帧
    int bestIndex = 0;
    int bestArea = 0;
    for(int i = 0; i < count; ++i) {
        if(!reader.jumpToImage(i)) {
            continue;
        }
        const QSize s = reader.size();
        const int area = s.width() * s.height();
        if(area > bestArea) {
            bestArea = area;
            bestIndex = i;
        }
    }
    if(bestArea <= 0) {
        qWarning() << "ImageStatic::loadICO() - No valid frames in ICO file:" << mPath;
        return;
    }

    if(!reader.jumpToImage(bestIndex)) {
        qWarning() << "ImageStatic::loadICO() - Failed to select frame:" << mPath;
        return;
    }

    QImage img;
    if(!reader.read(&img) || img.isNull()) {
        qWarning() << "ImageStatic::loadICO() - Failed to decode frame:" << mPath;
        return;
    }

    image = std::make_shared<const QImage>(std::move(img));
    mLoaded = true;
}

QString ImageStatic::generateHash(QStringView str) noexcept {
    // 备份后缀只需唯一标识，qHash 足够，避免 MD5 开销
    return QString::number(static_cast<qulonglong>(qHash(str)), 16);
}

int ImageStatic::getSaveQuality(QStringView ext) noexcept {
    // Qt 6: 使用 QStringView 避免字符串复制
    if (ext.compare(u"png", Qt::CaseInsensitive) == 0) {
        return 70;  // PNG 压缩级别 3-6 通常与 9 性能相当
    }
    if (ext.compare(u"webp", Qt::CaseInsensitive) == 0) {
        return 100;  // WebP 永远最高质量
    }
    if (ext.compare(u"jpg", Qt::CaseInsensitive) == 0 ||
        ext.compare(u"jpeg", Qt::CaseInsensitive) == 0) {
        return settings->ImageSaveQuality();
    }
    return settings->ImageSaveQuality();  // 默认使用全局图片质量设置
}

bool ImageStatic::save(QString destPath) {
    // Qt 6: 使用 QSaveFile 实现原子写入，更安全
    const QString ext = QFileInfo(destPath).suffix();
    const int quality = getSaveQuality(ext);
    
    const bool originalExists = QFile::exists(destPath);
    QString backupPath;
    
    // 备份原文件
    if(originalExists) {
        backupPath = destPath + "_" + generateHash(destPath);
        QFile::remove(backupPath);
        if(!QFile::copy(destPath, backupPath)) {
            qWarning() << "ImageStatic::save() - Could not create backup:" << destPath;
            return false;
        }
    }
    
    // 获取要保存的图像
    const QImage *imgToSave = isEdited() ? imageEdited.get() : image.get();
    if(!imgToSave || imgToSave->isNull()) {
        qWarning() << "ImageStatic::save() - No valid image to save";
        if(originalExists && !backupPath.isEmpty()) {
            QFile::remove(backupPath);
        }
        return false;
    }
    
    // Qt 6: 使用 QSaveFile 确保原子写入
    QSaveFile saveFile(destPath);
    if(!saveFile.open(QIODevice::WriteOnly)) {
        qWarning() << "ImageStatic::save() - Cannot open file for writing:" << destPath;
        if(originalExists && !backupPath.isEmpty()) {
            QFile::remove(backupPath);
        }
        return false;
    }
    
    // 以 UTF-8 编码解析格式，而不是 Latin1，以兼容 Windows 上的中文路径和元数据
    QImageWriter writer(&saveFile, ext.toUtf8());
    writer.setQuality(quality);
    
    // 保留原始图片的元数据（特别是 EXIF 中的文本字段）
    // 这修复了编辑后保存图片时中文标题变乱码的问题
    if(destPath == mPath || isEdited()) {
        for(auto it = mTextMetadata.constBegin(); it != mTextMetadata.constEnd(); ++it) {
            writer.setText(it.key(), it.value());
        }
    }
    
    const bool success = writer.write(*imgToSave);
    
    if(success) {
        // 提交写入
        if(!saveFile.commit()) {
            qWarning() << "ImageStatic::save() - Failed to commit file:" << destPath;
            if(originalExists && !backupPath.isEmpty()) {
                QFile::remove(backupPath);
            }
            return false;
        }
        
        // 保存成功，删除备份
        if(originalExists && !backupPath.isEmpty()) {
            QFile::remove(backupPath);
        }
        
        // 交换编辑图像
        if(isEdited()) {
            image.swap(imageEdited);
            discardEditedImage();
        }
        
        // 刷新文档信息
        if(destPath == mPath) {
            mDocInfo->refresh();
        }
        
        return true;
    }
    // 保存失败，回滚
    saveFile.cancelWriting();
    qWarning() << "ImageStatic::save() - Write failed:" << writer.errorString();
    
    if(originalExists && !backupPath.isEmpty()) {
        QFile::remove(destPath);
        QFile::copy(backupPath, destPath);
        QFile::remove(backupPath);
    }
    return false;
}

bool ImageStatic::save() {
    return save(mPath);
}

const QImage& ImageStatic::currentImage() const noexcept {
    static const QImage nullImage;
    if(imageEdited) {
        return *imageEdited;
    }
    if(image) {
        return *image;
    }
    return nullImage;
}

void ImageStatic::getPixmap(QPixmap& outPixmap) const {
    const QImage &img = currentImage();
    if(img.isNull()) {
        outPixmap = QPixmap();
        return;
    }

    // 缓存命中：QImage::cacheKey() 在内容变化时自动改变（编辑/撤销/保存后）
    const qint64 key = img.cacheKey();
    if(key == mCachedPixmapKey && !mCachedPixmap.isNull()) {
        outPixmap = mCachedPixmap; // 隐式共享，浅拷贝
        return;
    }

    mCachedPixmap = QPixmap::fromImage(img);
    mCachedPixmapKey = key;
    outPixmap = mCachedPixmap;
}

std::shared_ptr<const QImage> ImageStatic::getSourceImage() const noexcept {
    return image;
}

std::shared_ptr<const QImage> ImageStatic::getImage() const noexcept {
    return isEdited() ? imageEdited : image;
}

int ImageStatic::height() const noexcept {
    const QImage &img = currentImage();
    return img.isNull() ? 0 : img.height();
}

int ImageStatic::width() const noexcept {
    const QImage &img = currentImage();
    return img.isNull() ? 0 : img.width();
}

QSize ImageStatic::size() const noexcept {
    const QImage &img = currentImage();
    return img.isNull() ? QSize() : img.size();
}

bool ImageStatic::setEditedImage(std::unique_ptr<const QImage> imageEditedNew) {
    if(!imageEditedNew || imageEditedNew->width() <= 0 || imageEditedNew->height() <= 0) {
        return false;
    }
    
    discardEditedImage();
    imageEdited = std::shared_ptr<const QImage>(imageEditedNew.release());
    mEdited = true;
    return true;
}

bool ImageStatic::discardEditedImage() noexcept {
    if(imageEdited) {
        imageEdited.reset();
        mEdited = false;
        return true;
    }
    return false;
}

void ImageStatic::crop(QRect newRect) {
    const QImage &src = currentImage();

    if (src.isNull() || !newRect.isValid()) {
        return;
    }

    newRect = newRect.intersected(src.rect());
    if (newRect.isEmpty()) {
        return;
    }

    auto croppedImage = std::make_unique<QImage>(src.copy(newRect));

    if (!croppedImage || croppedImage->isNull()) {
        return;
    }

    setEditedImage(std::move(croppedImage));
}
