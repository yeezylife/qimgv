#pragma once

#include <QString>
#include <QStringView>

#ifdef _WIN32
    #define StdString std::wstring
    #define CharType wchar_t
#else
    #define StdString std::string
    #define CharType char
#endif

int clamp(int x, int lower, int upper);
int probeOS();

// 新增：使用 QStringView 的版本，避免不必要的字符串复制
StdString toStdString(QStringView str);
QString fromStdString(StdString str);

// 保持向后兼容：原有的 QString 版本调用 QStringView 版本
inline StdString toStdString(QString str) { 
    return toStdString(QStringView(str)); 
}
