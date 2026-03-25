#include "loaderrunnable.h"

LoaderRunnable::LoaderRunnable(const QString& _path)
    : path(_path)
{
}

void LoaderRunnable::run() {
    auto image = ImageFactory::createImage(path);

    // 🚀 move 减少 shared_ptr 原子操作
    emit finished(std::move(image), path);
}