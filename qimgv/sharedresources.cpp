#include "sharedresources.h"

// TODO: is there a point in doing this? qt does implicit sharing for pixmaps? test

SharedResources *shrRes = nullptr;

SharedResources::SharedResources()
{
}

SharedResources::~SharedResources() {
    // 【修正说明】
    // 1. 移除了 delete shrRes;
    //    原因：shrRes 指向 this，在析构函数中 delete this 会导致双重释放崩溃。
    // 2. 添加了 shrRes = nullptr;
    //    原因：防止外部再次调用 getInstance() 时返回悬空指针。
    // 3. 补充了成员变量的清理。
    //    原因：原代码存在内存泄漏，单例销毁时未释放 QPixmap 内存。
    
    if (shrRes == this) {
        shrRes = nullptr;
    }

    delete mLoadingIcon72;
    mLoadingIcon72 = nullptr;
    
    delete mLoadingErrorIcon72;
    mLoadingErrorIcon72 = nullptr;
}

QPixmap *SharedResources::getPixmap(ShrIcon icon, qreal dpr) {
    QPixmap *pixmap;
    QString path;
    if(icon == ShrIcon::SHR_ICON_ERROR) {
        path = ":/res/icons/common/other/loading-error72.png";
        pixmap = mLoadingErrorIcon72;
    } else {
        path = ":/res/icons/common/other/loading72.png";
        pixmap = mLoadingIcon72;
    }
    if(pixmap)
        return pixmap;

    qreal pixmapDrawScale;
    if(dpr >= (1.0 + 0.001)) {
        path.replace(".", "@2x.");
        pixmap = new QPixmap(path);
        if(dpr >= (2.0 - 0.001))
            pixmapDrawScale = dpr;
        else
            pixmapDrawScale = 2.0;
        pixmap->setDevicePixelRatio(pixmapDrawScale);
    } else {
        pixmap = new QPixmap(path);
    }
    if(icon == ShrIcon::SHR_ICON_ERROR)
        mLoadingErrorIcon72 = pixmap;
    else
        mLoadingIcon72 = pixmap;
    return pixmap;
}

SharedResources *SharedResources::getInstance() {
    if(!shrRes) {
        shrRes = new SharedResources();
    }
    return shrRes;
}
