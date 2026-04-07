#ifndef WINDOWSWATCHER_P_H
#define WINDOWSWATCHER_P_H

#include "windowswatcher.h"
#include "../directorywatcher_p.h"
#include <windows.h>
#include <QString>
#include <QDebug>

class ScopedHandle;
class WindowsWorker;

// 优化：使用 QStringLiteral 和 asprintf 减少临时 QString 对象的构造开销
static inline QString lastError()
{
    char buffer[1024];
    DWORD lastError = GetLastError(); // 保存原始错误码
    DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, lastError, LANG_SYSTEM_DEFAULT, buffer, sizeof(buffer), nullptr);
    if (res == 0) {
        return QString::number(lastError); // 使用保存的错误码
    }
    return QString::asprintf("%s::%d: %s", __FILE__, __LINE__, buffer);
}

class WindowsWatcherPrivate : public DirectoryWatcherPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(WindowsWatcher)

public:
    explicit WindowsWatcherPrivate(WindowsWatcher* qq);
    ScopedHandle requestDirectoryHandle(const QString& path);

public slots:
    void dispatchNotify(const QString& fileName, DWORD action);

};

#endif // WINDOWSWATCHER_P_H