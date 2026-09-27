#pragma once

#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QHash>
#include <QStringView>
#include <optional>
#include <memory>
#include "image.h"
#include "utils/imagelib.h"

class ImageStatic : public Image {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ImageStatic)

public:
    // 任意线程可调用：解码限额已由调用方在 GUI 线程快照
    ImageStatic(std::unique_ptr<DocumentInfo> info, int allocationLimitMB);
    ~ImageStatic() override = default;
    
    using Image::save;

    // 使用 std::optional 表示可能失败的操作
    void getPixmap(QPixmap& outPixmap) const override;
    std::shared_ptr<const QImage> getSourceImage() const noexcept;
    std::shared_ptr<const QImage> getImage() const noexcept override;

    // const 成员函数
    int height() const noexcept override;
    int width() const noexcept override;
    QSize size() const noexcept override;

    // 编辑相关
    bool setEditedImage(std::unique_ptr<const QImage> imageEditedNew);
    bool discardEditedImage() noexcept;
    bool isEdited() const noexcept { return mEdited; }

public slots:
    void crop(QRect newRect);
    bool save() override;
    bool save(QString destPath) override;

private:
    void loadWith(int allocationLimitMB);
    void loadGeneric(int allocationLimitMB);
    void loadICO();
    const QImage &currentImage() const noexcept;
    static QString generateHash(QStringView str) noexcept;
    static int getSaveQuality(QStringView ext) noexcept;
    
    std::shared_ptr<const QImage> image;
    std::shared_ptr<const QImage> imageEdited;

    // 加载时缓存的文本元数据（EXIF），保存时复用，避免每次重新打开原文件
    QHash<QString, QString> mTextMetadata;

    // QPixmap 缓存：QImage::cacheKey() 变化时自动失效
    mutable QPixmap mCachedPixmap;
    mutable qint64 mCachedPixmapKey = -1;
};
