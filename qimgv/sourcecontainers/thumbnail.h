#pragma once

#include <QString>
#include <QPixmap>
#include <memory>

class Thumbnail {
public:
    // 调整参数顺序：int 分隔两个 QString；shared_ptr 改为 const 引用
    Thumbnail(const QString &_name, int _size, const QString &_info, const std::shared_ptr<QPixmap> &_pixmap);

    QString name() const;
    QString info() const;
    int size() const;
    bool hasAlphaChannel() const;
    std::shared_ptr<QPixmap> pixmap() const;

private:
    QString mName;
    QString mInfo;
    std::shared_ptr<QPixmap> mPixmap;
    int mSize;
    bool mHasAlphaChannel;
};