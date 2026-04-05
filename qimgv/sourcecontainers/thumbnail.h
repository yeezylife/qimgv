#pragma once

#include <QString>
#include <QPixmap>
#include <memory>

class Thumbnail {
public:
    Thumbnail(const QString &_name, int _size, const QString &_info, std::shared_ptr<const QPixmap> _pixmap);

    QString name() const;
    QString info() const;
    int size() const;
    bool hasAlphaChannel() const;
    std::shared_ptr<const QPixmap> pixmap() const;

private:
    QString mName;
    QString mInfo;
    std::shared_ptr<const QPixmap> mPixmap;
    int mSize;
    bool mHasAlphaChannel;
};