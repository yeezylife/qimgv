#include "sharedresources.h"
#include <QFileInfo>

SharedResources& SharedResources::getInstance()
{
    // C++11 线程安全懒加载
    static SharedResources instance;
    return instance;
}

QPixmap& SharedResources::getPixmap(ShrIcon icon, qreal dpr)
{
    std::unique_ptr<QPixmap>& targetPixmap =
        (icon == SHR_ICON_ERROR) ? mLoadingErrorIcon72 : mLoadingIcon72;

    if (targetPixmap) {
        return *targetPixmap;
    }

    QString basePath = (icon == SHR_ICON_ERROR)
        ? ":/res/icons/common/other/loading-error72.png"
        : ":/res/icons/common/other/loading72.png";

    QString path = basePath;
    qreal targetDpr = 1.0;

    if (dpr >= 1.001) {
        QFileInfo fi(basePath);
        path = fi.path() + "/" + fi.completeBaseName() + "@2x." + fi.suffix();
        targetDpr = (dpr >= 1.999) ? dpr : 2.0;
    }

    auto pixmap = std::make_unique<QPixmap>(path);

    if (!pixmap->isNull() && targetDpr != 1.0) {
        pixmap->setDevicePixelRatio(targetDpr);
    }

    targetPixmap = std::move(pixmap);
    return *targetPixmap;
}

const QPixmap& SharedResources::getPixmap(ShrIcon icon, qreal dpr) const
{
    return const_cast<SharedResources*>(this)->getPixmap(icon, dpr);
}
