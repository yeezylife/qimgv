#pragma once

#include "image.h"
#include <QMovie>
#include <QTimer>
#include <memory>
#include <atomic>

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

    // ✅ 帧缓存（无锁线程安全版本，使用 C++20 atomic<shared_ptr>）
    std::atomic<std::shared_ptr<const QImage>> cachedFrame;

private slots:
    void onFrameChanged(int frameNumber);
};