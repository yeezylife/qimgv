#include "appversion.h"

const QVersionNumber& appVersion()
{
    static const QVersionNumber v(1, 0, 3);
    return v;
}
