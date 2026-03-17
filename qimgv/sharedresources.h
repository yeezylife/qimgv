#pragma once

#include <QPixmap>
#include <QtGlobal>
#include <memory>   // 引入 std::unique_ptr

enum ShrIcon {
    SHR_ICON_ERROR,
    SHR_ICON_LOADING
};

class SharedResources
{
public:
    static SharedResources& getInstance();                // 返回引用
    ~SharedResources() = default;                         // 默认析构即可

    QPixmap& getPixmap(ShrIcon icon, qreal dpr);          // 返回引用
    const QPixmap& getPixmap(ShrIcon icon, qreal dpr) const; // const 重载，保证只读

private:
    SharedResources() = default;                          // 私有构造函数，强制使用单例
    Q_DISABLE_COPY(SharedResources)                       // 禁止拷贝

    mutable std::unique_ptr<QPixmap> mLoadingIcon72;      // 使用智能指针管理资源
    mutable std::unique_ptr<QPixmap> mLoadingErrorIcon72;
};

// 保留全局引用，用于兼容旧代码
extern SharedResources& shrRes;