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
    void onWorkerRequestPath(const QString& path);

private:
    // 注意：oldFileName仅在dispatchNotify槽函数中读写，该槽函数在worker线程中执行。
    // 由于重命名事件总是成对出现（FILE_ACTION_RENAMED_OLD_NAME后跟FILE_ACTION_RENAMED_NEW_NAME），
    // 且Qt信号槽连接保证同一线程内的顺序调用，因此线程安全。
    QString oldFileName;
};

#endif // WINDOWSWATCHER_P_H