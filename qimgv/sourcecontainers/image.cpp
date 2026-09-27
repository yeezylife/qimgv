#include "image.h"

Image::Image(std::unique_ptr<DocumentInfo> info)
    : mDocInfo(std::move(info)),
      mPath(mDocInfo ? mDocInfo->filePath() : QString())
{
}

// 【修改1】移除了析构函数的定义，由编译器自动生成内联默认析构函数
