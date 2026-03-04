#include "stuff.h"
#include <algorithm> // std::clamp

// 使用 C++17 标准库
int clamp(int x, int lower, int upper) {
    return std::clamp(x, lower, upper);
}

// 既然只在 Windows 上运行，直接返回 2 即可
// 这也消除了不必要的宏展开逻辑
int probeOS() {
    return 2;
}

// 直接转换，无需任何宏判断
StdString toStdString(const QString& str) {
    return str.toStdWString();
}

QString fromStdString(const StdString& str) {
    return QString::fromStdWString(str);
}
