#pragma once

#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QCryptographicHash>
#include <QIcon>
#include <QStringView>
#include <optional>
#include <memory>
#include "image.h"
#include "utils/imagelib.h"

class ImageStatic : public Image {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ImageStatic)

public:
    explicit ImageStatic(QString path);
    explicit ImageStatic(std::unique_ptr<DocumentInfo> info);
    ~ImageStatic() override = default;
    
    using Image::save;

    // 使用 std::optional 表示可能失败的操作
    std::unique_ptr<QPixmap> getPixmap() const override;
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
    void load() override;
    void loadGeneric();
    void loadICO();
    static QString generateHash(QStringView str) noexcept;
    static int getSaveQuality(QStringView ext) noexcept;
    
    std::shared_ptr<const QImage> image;
    std::shared_ptr<const QImage> imageEdited;
};