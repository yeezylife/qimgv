#pragma once

#include "image.h"
#include <QMovie>
#include <QTimer>

class ImageAnimated : public Image {
public:
    ImageAnimated(QString _path);
    ImageAnimated(std::unique_ptr<DocumentInfo> _info);
    ~ImageAnimated();
    
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
    bool save();
    bool save(QString destPath);

signals:
    void frameChanged(QPixmap*);

private:
    void load();
    QSize mSize;
    int mFrameCount;
    std::shared_ptr<QMovie> movie;
    void loadMovie();
};
