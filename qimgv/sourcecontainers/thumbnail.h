#pragma once

#include <QString>
#include <QPixmap>
#include <memory>

class Thumbnail {
public:
    // 优化：QString 使用 const 引用传递，避免不必要的拷贝
    Thumbnail(const QString &_name, const QString &_info, int _size, std::shared_ptr<QPixmap> _pixmap);
    
    // 优化：添加 const 修饰符，符合常量正确性规范
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
