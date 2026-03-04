#ifndef WINDOWSWATCHER_P_H
#define WINDOWSWATCHER_P_H

#include "../directorywatcher_p.h"
#include <windows.h>
#include <QString>
#include <QDebug>

class WindowsWatcher;
class WindowsWorker;

// --- 新增：移到这里，设为 static inline ---
static inline QString lastError() {
    char buffer[1024];
    DWORD lastError = GetLastError();
    int res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                             nullptr,
                             lastError,
                             LANG_SYSTEM_DEFAULT,
                             buffer,
                             sizeof(buffer),
                             nullptr);
    QString line = QString(__FILE__) + "::" + QString::number(__LINE__) + ": ";
    return res == 0 ? QString::number(GetLastError()) : line + buffer;
}
// -----------------------------------------

class WindowsWatcherPrivate : public DirectoryWatcherPrivate {
    Q_DECLARE_PUBLIC(WindowsWatcher)
public:
    explicit WindowsWatcherPrivate(WindowsWatcher* qq);
    
    HANDLE requestDirectoryHandle(const QString& path);
    void dispatchNotify(const QString& fileName, DWORD action);  // 修改：传递字符串和动作

    QString oldFileName;
};

#endif // WINDOWSWATCHER_P_H
