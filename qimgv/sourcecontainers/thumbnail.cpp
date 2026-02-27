#include "thumbnail.h"

// 优化：使用成员初始化列表，并在初始化列表中处理逻辑，确保所有成员都被初始化
Thumbnail::Thumbnail(const QString &_name, const QString &_info, int _size, std::shared_ptr<QPixmap> _pixmap)
    : mName(_name),
      mInfo(_info),
      mPixmap(_pixmap),
      mSize(_size),
      // 修复：如果 _pixmap 为空，mHasAlphaChannel 必须初始化为 false，否则原代码会导致该变量值不确定
      mHasAlphaChannel(_pixmap ? _pixmap->hasAlphaChannel() : false)
{
}

QString Thumbnail::name() const {
    return mName;
}

QString Thumbnail::info() const {
    return mInfo;
}

int Thumbnail::size() const {
    return mSize;
}

bool Thumbnail::hasAlphaChannel() const {
    return mHasAlphaChannel;
}

std::shared_ptr<QPixmap> Thumbnail::pixmap() const {
    return mPixmap;
}
