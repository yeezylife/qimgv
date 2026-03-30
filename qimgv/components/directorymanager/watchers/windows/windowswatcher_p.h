#ifndef WINDOWSWATCHER_P_H
#define WINDOWSWATCHER_P_H

#include “windowswatcher.h”
#include “…/directorywatcher_p.h”
#include <windows.h>
#include <QString>
#include <QDebug>

class WindowsWorker;

// 优化：使用 QStringLiteral 和 asprintf 减少临时 QString 对象的构造开销
static inline QString lastError()
{
char buffer[1024];
DWORD lastError = GetLastError();
DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, lastError, LANG_SYSTEM_DEFAULT, buffer, sizeof(buffer), nullptr);
if (res == 0) {
return QString::number(GetLastError());
}
return QString::asprintf(“%s::%d: %s”, FILE, LINE, buffer);
}

class WindowsWatcherPrivate : public DirectoryWatcherPrivate
{
Q_OBJECT
Q_DECLARE_PUBLIC(WindowsWatcher)

public:
explicit WindowsWatcherPrivate(WindowsWatcher* qq);
HANDLE requestDirectoryHandle(const QString& path);

public slots:
void dispatchNotify(const QString& fileName, DWORD action);

private:
QString oldFileName;
};

#endif // WINDOWSWATCHER_P_H