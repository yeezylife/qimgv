#include "loaderrunnable.h"

LoaderRunnable::LoaderRunnable(const QString& _path) 
    : path(_path) 
{
}

void LoaderRunnable::run() {
    auto image = ImageFactory::createImage(path);
    // 🚀 优化：使用 std::move 减少 shared_ptr 引用计数的原子操作开销
    emit finished(std::move(image), path);
}
