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
    load();
}

ImageStatic::ImageStatic(std::unique_ptr<DocumentInfo> info)
    : Image(std::move(info))
{
    load();
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
    /* QImageReader::read() seems more reliable than just reading via QImage.
     * For example: "Invalid JPEG file structure: two SOF markers"
     * QImageReader::read() returns false, but still reads an image. Meanwhile QImage just fails.
     *
     * tldr: qimage bad
     */
    // 使用 UTF-8 而不是 Latin1，以兼容 Windows 上的中文路径和元数据
    QImageReader reader(mPath, mDocInfo->format().toUtf8());
    
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    reader.setAllocationLimit(settings->memoryAllocationLimit());
#endif
    
    // Qt 6: 使用 auto 和更简洁的 API
    auto imagePtr = std::make_unique<QImage>();
    
    if(!reader.read(imagePtr.get())) {
        qWarning() << "ImageStatic::loadGeneric() - Failed to read image:" << mPath 
                   << "Error:" << reader.errorString();
        // 即使失败也尝试使用已读取的数据
        if(imagePtr->isNull()) {
            return;
        }
    }
    
    // EXIF 旋转处理
    imagePtr = ImageLib::exifRotated(std::move(imagePtr), mDocInfo->exifOrientation());
    if(!imagePtr) {
        return;
    }
    
    // Format_Mono 转换 - Qt 6 使用更现代的写法
    if(imagePtr->format() == QImage::Format_Mono) {
        image = std::make_shared<const QImage>(
            imagePtr->convertToFormat(QImage::Format_Grayscale8)
        );
    } else {
        image = std::shared_ptr<const QImage>(imagePtr.release());
    }
    
    mLoaded = true;
}

void ImageStatic::loadICO() {
    // ICO 加载逻辑
    const QIcon icon(mPath);
    if(icon.availableSizes().isEmpty()) {
        qWarning() << "ImageStatic::loadICO() - No sizes available in ICO file:" << mPath;
        return;
    }
    
    // Qt 6: 使用 std::max_element 更优雅
    const auto sizes = icon.availableSizes();
    const auto maxSizeIt = std::max_element(
        sizes.begin(), sizes.end(),
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
        return 30;  // PNG 压缩级别 3-6 通常与 9 性能相当
    }
    if (ext.compare(u"jpg", Qt::CaseInsensitive) == 0 ||
        ext.compare(u"jpeg", Qt::CaseInsensitive) == 0) {
        return settings->JPEGSaveQuality();
    }
    return 95;  // 默认质量
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
        QImageReader reader(mPath, mDocInfo->format().toUtf8());
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
    } else {
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
}

bool ImageStatic::save() {
    return save(mPath);
}

std::unique_ptr<QPixmap> ImageStatic::getPixmap() const {
    const QImage *img = isEdited() ? imageEdited.get() : image.get();
    if(!img || img->isNull()) {
        return nullptr;
    }
    
    auto pix = std::make_unique<QPixmap>();
    pix->convertFromImage(*img, Qt::NoFormatConversion);
    return pix;
}

std::shared_ptr<const QImage> ImageStatic::getSourceImage() const noexcept {
    return image;
}

std::shared_ptr<const QImage> ImageStatic::getImage() const noexcept {
    return isEdited() ? imageEdited : image;
}

int ImageStatic::height() const noexcept {
    const QImage *img = isEdited() ? imageEdited.get() : image.get();
    return img ? img->height() : 0;
}

int ImageStatic::width() const noexcept {
    const QImage *img = isEdited() ? imageEdited.get() : image.get();
    return img ? img->width() : 0;
}

QSize ImageStatic::size() const noexcept {
    const QImage *img = isEdited() ? imageEdited.get() : image.get();
    return img ? img->size() : QSize();
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
    if(!image || image->isNull() || !newRect.isValid()) {
        return;
    }
    
    // 确保裁剪区域在图像范围内
    newRect = newRect.intersected(image->rect());
    if(newRect.isEmpty()) {
        return;
    }
    
    // 创建裁剪后的图像
    std::unique_ptr<const QImage> croppedImage = std::make_unique<QImage>(image->copy(newRect));
    if(!croppedImage || croppedImage->isNull()) {
        return;
    }
    
    // 设置为编辑后的图像
    setEditedImage(std::move(croppedImage));
}
