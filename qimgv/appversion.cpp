#include "appversion.h"

static const QVersionNumber& getAppVersion()
{
    static const QVersionNumber v(1, 0, 3);
    return v;
}

const QVersionNumber& appVersion = getAppVersion();
