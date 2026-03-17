#include "sharedresources.h"
#include <QFileInfo>

// 定义全局引用，直接绑定到单例实例
SharedResources& shrRes = SharedResources::getInstance();

SharedResources& SharedResources::getInstance()
{
    // 线程安全的局部静态实例（C++11 起）
    static SharedResources instance;
    return instance;
}

QPixmap& SharedResources::getPixmap(ShrIcon icon, qreal dpr)
{
    // 根据图标选择对应的成员指针
    std::unique_ptr<QPixmap>& targetPixmap =
        (icon == SHR_ICON_ERROR) ? mLoadingErrorIcon72 : mLoadingIcon72;

    // 已缓存直接返回
    if (targetPixmap) {
        return *targetPixmap;
    }

    // 确定基础路径
    QString basePath = (icon == SHR_ICON_ERROR)
        ? ":/res/icons/common/other/loading-error72.png"
        : ":/res/icons/common/other/loading72.png";

    QString path = basePath;
    qreal targetDpr = 1.0;

    // 判断是否需要高分辨率图片
    if (dpr >= 1.0 + 0.001) {
        QFileInfo fi(basePath);
        path = fi.path() + "/" + fi.completeBaseName() + "@2x." + fi.suffix();
        targetDpr = (dpr >= 2.0 - 0.001) ? dpr : 2.0;
    }

    // 加载图片
    auto pixmap = std::make_unique<QPixmap>(path);

    if (!pixmap->isNull() && targetDpr != 1.0) {
        pixmap->setDevicePixelRatio(targetDpr);
    }

    targetPixmap = std::move(pixmap);
    return *targetPixmap;
}

const QPixmap& SharedResources::getPixmap(ShrIcon icon, qreal dpr) const
{
    // const 重载调用非 const 版本，保证只读
    return const_cast<SharedResources*>(this)->getPixmap(icon, dpr);
}