#pragma once

#include "image.h"
#include <QMovie>
#include <memory>

class ImageAnimated final : public Image {
public:
    explicit ImageAnimated(QString _path);
    explicit ImageAnimated(std::unique_ptr<DocumentInfo> _info);
    ~ImageAnimated() override = default;

    using Image::save;

    void getPixmap(QPixmap& outPixmap) const override;
    std::shared_ptr<const QImage> getImage() const override;
    std::shared_ptr<QMovie> getMovie();
    int height() const override;
    int width() const override;
    QSize size() const override;

    bool isEditable();
    bool isEdited();

    int frameCount();

public slots:
    bool save() override;
    bool save(QString destPath) override;

signals:
    void frameChanged(QPixmap*);

private:
    void load() override;
    void loadMovie();

    QSize mSize;
    int mFrameCount = 0;
    std::shared_ptr<QMovie> movie;

    // ✅ 使用普通 shared_ptr + atomic free functions
    std::shared_ptr<const QImage> cachedFrame;

private slots:
    void onFrameChanged(int frameNumber);
};