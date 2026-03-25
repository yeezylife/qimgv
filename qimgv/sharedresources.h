#pragma once

#include <QPixmap>
#include <QtGlobal>
#include <memory>

enum ShrIcon {
    SHR_ICON_ERROR,
    SHR_ICON_LOADING
};

class SharedResources
{
public:
    static SharedResources& getInstance();                // 单例引用
    ~SharedResources() = default;

    QPixmap& getPixmap(ShrIcon icon, qreal dpr);
    const QPixmap& getPixmap(ShrIcon icon, qreal dpr) const;

private:
    SharedResources() = default;
    Q_DISABLE_COPY(SharedResources)

    mutable std::unique_ptr<QPixmap> mLoadingIcon72;
    mutable std::unique_ptr<QPixmap> mLoadingErrorIcon72;
};

// 全局引用声明（不初始化）
extern SharedResources& shrRes;
