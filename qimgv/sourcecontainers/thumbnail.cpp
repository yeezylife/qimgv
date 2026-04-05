#include "thumbnail.h"

Thumbnail::Thumbnail(const QString &_name, int _size, const QString &_info, std::shared_ptr<const QPixmap> _pixmap)
    : mName(_name),
      mInfo(_info),
      mPixmap(std::move(_pixmap)),
      mSize(_size),
      mHasAlphaChannel(false)
{
}

QString Thumbnail::name() const { return mName; }
QString Thumbnail::info() const { return mInfo; }
int Thumbnail::size() const { return mSize; }
bool Thumbnail::hasAlphaChannel() const { return mHasAlphaChannel; }
std::shared_ptr<const QPixmap> Thumbnail::pixmap() const { return mPixmap; }
