#include "sharedresources.h"
#include <QFileInfo>

// 定义全局指针，初始为 nullptr
SharedResources *shrRes = nullptr;

SharedResources::SharedResources()
{
    // 构造函数私有，由内部静态实例调用
}

SharedResources::~SharedResources()
{
    // 如果全局指针指向当前实例，将其置空，防止悬空
    if (shrRes == this) {
        shrRes = nullptr;
    }

    // 清理资源
    delete mLoadingIcon72;
    mLoadingIcon72 = nullptr;
    delete mLoadingErrorIcon72;
    mLoadingErrorIcon72 = nullptr;
}

SharedResources *SharedResources::getInstance()
{
    // 线程安全的局部静态实例（C++11 起）
    static SharedResources instance;
    // 为了兼容外部全局指针 shrRes，将其指向实例
    if (shrRes != &instance) {
        shrRes = &instance;
    }
    return &instance;
}

QPixmap *SharedResources::getPixmap(ShrIcon icon, qreal dpr)
{
    QMutexLocker locker(&mutex);  // 加锁，确保线程安全

    // 根据图标选择对应的成员指针
    QPixmap*& targetPixmap = (icon == SHR_ICON_ERROR) ? mLoadingErrorIcon72 : mLoadingIcon72;

    // 如果已经加载过，直接返回
    if (targetPixmap) {
        return targetPixmap;
    }

    // 确定基础路径
    QString basePath;
    if (icon == SHR_ICON_ERROR) {
        basePath = ":/res/icons/common/other/loading-error72.png";
    } else {
        basePath = ":/res/icons/common/other/loading72.png";
    }

    QString path = basePath;
    qreal targetDpr = 1.0;

    // 判断是否需要高分辨率图片
    if (dpr >= 1.0 + 0.001) {
        // 使用 QFileInfo 安全地构造 @2x 路径
        QFileInfo fi(basePath);
        path = fi.path() + "/" + fi.completeBaseName() + "@2x." + fi.suffix();
        targetDpr = (dpr >= 2.0 - 0.001) ? dpr : 2.0;  // 与原逻辑保持一致
    }

    // 加载图片
    QPixmap *pixmap = new QPixmap(path);

    // 如果图片有效且需要设置设备像素比，则设置
    if (!pixmap->isNull() && targetDpr != 1.0) {
        pixmap->setDevicePixelRatio(targetDpr);
    }

    // 缓存结果（即使图片为空，也缓存，以保持与原行为一致）
    targetPixmap = pixmap;
    return pixmap;
}