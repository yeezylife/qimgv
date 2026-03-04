#pragma once

#include <QImage>
#include <QPixmap>
#include <QProcess>
#include <QFile>
#include "image.h"

class Video : public Image {
public:
    Video(QString _path);
    Video(std::unique_ptr<DocumentInfo> _info);

    std::unique_ptr<QPixmap> getPixmap() const override;
    std::shared_ptr<const QImage> getImage() const override;
    int height() const override;
    int width() const override;
    QSize size() const override;

public slots:
    bool save();
    bool save(QString destPath);


private:
    void load();

    uint srcWidth = 0;
    uint srcHeight = 0;
};
