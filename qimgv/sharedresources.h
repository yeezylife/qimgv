#pragma once

#include <QPixmap>
#include <QMutex>
#include <QtGlobal>

enum ShrIcon {
    SHR_ICON_ERROR,
    SHR_ICON_LOADING
};

class SharedResources
{
public:
    static SharedResources* getInstance();
    ~SharedResources();

    QPixmap *getPixmap(ShrIcon icon, qreal dpr);

private:
    SharedResources();                                    // 私有构造函数，强制使用单例
    Q_DISABLE_COPY(SharedResources)                       // 禁止拷贝

    QPixmap *mLoadingIcon72 = nullptr;
    QPixmap *mLoadingErrorIcon72 = nullptr;
    QMutex mutex;                                          // 用于线程安全
};

// 保留全局指针，用于兼容旧代码
extern SharedResources *shrRes;