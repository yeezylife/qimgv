#include "appversion.h"

// 添加 noexcept 说明符，表示该函数不会抛出异常
static const QVersionNumber& getAppVersion() noexcept
{
    static const QVersionNumber v(1, 0, 3);
    return v;
}

const QVersionNumber& appVersion = getAppVersion();
