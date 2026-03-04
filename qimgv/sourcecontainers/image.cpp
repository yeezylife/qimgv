#include "image.h"

// 构造函数优化：QString 按值传递后移动，避免多余的深拷贝
Image::Image(QString path)
    : mDocInfo(std::make_unique<DocumentInfo>(path)),
      mPath(std::move(path))
{
    // mLoaded 和 mEdited 已在头文件中初始化，无需在此设置
}

Image::Image(std::unique_ptr<DocumentInfo> info)
    : mDocInfo(std::move(info)),
      mPath(mDocInfo ? mDocInfo->filePath() : QString())
{
}

// 【修改1】移除了析构函数的定义，由编译器自动生成内联默认析构函数
