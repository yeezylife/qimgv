#include "imagestatic.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDebug>
#include <QImageReader>
#include <QImageWriter>

ImageStatic::ImageStatic(QString path)
    : Image(std::move(path))
{
    loadImage();
}

ImageStatic::ImageStatic(std::unique_ptr<DocumentInfo> info)
    : Image(std::move(info))
{
    loadImage();
}

void ImageStatic::load() {
    if(isLoaded()) {
        return;
    }
    
    const auto mimeType = mDocInfo->mimeType().name();
    if(mimeType == "image/vnd.microsoft.icon") {
        loadICO();
    } else {
        loadGeneric();
    }
}

void ImageStatic::loadGeneric() {
    QImageReader reader(mPath);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    reader.setAllocationLimit(settings->memoryAllocationLimit());
#endif

    // 禁用 Qt 自动方向处理，统一由我们自己控制
    reader.setAutoTransform(false);

    QImage imageData;
    if(!reader.read(&imageData)) {
        // reader 失败但 imageData 可能仍然有效（Qt 的特性）
        if(imageData.isNull()) {
            return;
        }
    }

    QImage image = std::move(imageData);

    // ✅ 修复：只在合法 EXIF 范围内处理
    const int orientation = mDocInfo->exifOrientation();
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
}

void ImageStatic::loadICO() {
    // ICO 加载逻辑
    const QIcon icon(mPath);
    const auto sizes = icon.availableSizes();
    if(sizes.isEmpty()) {
        qWarning() << "ImageStatic::loadICO() - No sizes available in ICO file:" << mPath;
        return;
    }

    const auto maxSizeIt = std::max_element(
        sizes.constBegin(), sizes.constEnd(),
        [](const QSize &a, const QSize &b) { return a.width() < b.width(); }
    );

    const QPixmap iconPix = icon.pixmap(*maxSizeIt);
    if(!iconPix.isNull()) {
        image = std::make_shared<const QImage>(iconPix.toImage());
        mLoaded = true;
    }
}

QString ImageStatic::generateHash(QStringView str) noexcept {
    // Qt 6: QCryptographicHash 支持 QStringView
    const auto hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5);
    return hash.toHex();
}

int ImageStatic::getSaveQuality(QStringView ext) noexcept {
    // Qt 6: 使用 QStringView 避免字符串复制
    if (ext.compare(u"png", Qt::CaseInsensitive) == 0) {
        return 70;  // PNG 压缩级别 3-6 通常与 9 性能相当
    }
    if (ext.compare(u"jpg", Qt::CaseInsensitive) == 0 ||
        ext.compare(u"jpeg", Qt::CaseInsensitive) == 0) {
        return settings->JPEGSaveQuality();
    }
    return 96;  // 默认质量
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
        // 从原始文件读取元数据
        QImageReader reader(mPath);
        QStringList textKeys = reader.textKeys();
        for(const QString &key : textKeys) {
            QString value = reader.text(key);
            if(!value.isEmpty()) {
                writer.setText(key, value);
            }
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

    outPixmap = QPixmap::fromImage(img);
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
