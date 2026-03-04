#pragma once

#include <QString>
#include <string>

// 既然只在 Windows 上，直接定义类型，不再使用宏判断
// 使用 using 代替 #define 是更现代的 C++ 写法
using StdString = std::wstring;
using CharType = wchar_t;

// 函数声明
[[nodiscard]] int clamp(int x, int lower, int upper);
[[nodiscard]] int probeOS();

// 参数使用 const & 避免拷贝，提升性能
[[nodiscard]] StdString toStdString(const QString& str);
[[nodiscard]] QString fromStdString(const StdString& str);
