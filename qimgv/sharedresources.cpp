#include "sharedresources.h"

// 资源路径常量（避免重复构造 QString）
namespace {
    constexpr const char* PATH_LOADING      = ":/res/icons/common/other/loading72.png";
    constexpr const char* PATH_LOADING_2X   = ":/res/icons/common/other/loading72@2x.png";

    constexpr const char* PATH_ERROR        = ":/res/icons/common/other/loading-error72.png";
    constexpr const char* PATH_ERROR_2X     = ":/res/icons/common/other/loading-error72@2x.png";
}

SharedResources& SharedResources::getInstance() noexcept
{
    static SharedResources instance;
    return instance;
}

QPixmap& SharedResources::getPixmap(ShrIcon icon, qreal dpr)
{
    // 指针选择提前完成，减少后续分支
    std::unique_ptr<QPixmap>* target =
        (icon == SHR_ICON_ERROR) ? &mLoadingErrorIcon72 : &mLoadingIcon72;

    if (*target) {
        return **target;
    }

    // DPR 判断（避免重复比较）
    const bool highDpr = (dpr >= 1.001);

    const char* path;
    qreal targetDpr = 1.0;

    if (icon == SHR_ICON_ERROR) {
        path = highDpr ? PATH_ERROR_2X : PATH_ERROR;
    } else {
        path = highDpr ? PATH_LOADING_2X : PATH_LOADING;
    }

    if (highDpr) {
        targetDpr = (dpr >= 1.999) ? dpr : 2.0;
    }

    auto pixmap = std::make_unique<QPixmap>(QString::fromUtf8(path));

    if (!pixmap->isNull() && targetDpr != 1.0) {
        pixmap->setDevicePixelRatio(targetDpr);
    }

    *target = std::move(pixmap);
    return **target;
}

const QPixmap& SharedResources::getPixmap(ShrIcon icon, qreal dpr) const
{
    return const_cast<SharedResources*>(this)->getPixmap(icon, dpr);
}