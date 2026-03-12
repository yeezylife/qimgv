#pragma once

#include "image.h"
#include <QMovie>
#include <QTimer>
#include <memory>

// 将类标记为 final 可以帮助编译器进行去虚化优化（devirtualization）
class ImageAnimated final : public Image {
public:
    explicit ImageAnimated(QString _path);
    explicit ImageAnimated(std::unique_ptr<DocumentInfo> _info);
    ~ImageAnimated() override = default; // 修复 modernize-use-equals-default

    using Image::save;

    std::unique_ptr<QPixmap> getPixmap() const override;
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
    // 依然保留 override，但构造函数中通过类名显式调用
    void load() override;
    void loadMovie();

    QSize mSize;
    int mFrameCount = 0;
    std::shared_ptr<QMovie> movie;
};