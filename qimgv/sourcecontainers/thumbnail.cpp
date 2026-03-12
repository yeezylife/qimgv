#include "thumbnail.h"

Thumbnail::Thumbnail(const QString &_name, int _size, const QString &_info, const std::shared_ptr<QPixmap> &_pixmap)
    : mName(_name),
      mInfo(_info),
      mPixmap(_pixmap),
      mSize(_size),
      mHasAlphaChannel(_pixmap ? _pixmap->hasAlphaChannel() : false)
{
}

QString Thumbnail::name() const { return mName; }
QString Thumbnail::info() const { return mInfo; }
int Thumbnail::size() const { return mSize; }
bool Thumbnail::hasAlphaChannel() const { return mHasAlphaChannel; }
std::shared_ptr<QPixmap> Thumbnail::pixmap() const { return mPixmap; }