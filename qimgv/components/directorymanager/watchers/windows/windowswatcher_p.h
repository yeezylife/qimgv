#ifndef WINDOWSWATCHER_P_H
#define WINDOWSWATCHER_P_H

#include "windowswatcher.h"                // 使 WindowsWatcher 类型完整
#include "../directorywatcher_p.h"
#include <windows.h>
#include <QString>
#include <QDebug>

class WindowsWorker;                        // WindowsWorker 仍保持前向声明即可

// 辅助函数：获取最后一次错误信息
static inline QString lastError() {
    char buffer[1024];
    DWORD lastError = GetLastError();
    DWORD res = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                               nullptr,
                               lastError,
                               LANG_SYSTEM_DEFAULT,
                               buffer,
                               sizeof(buffer),
                               nullptr);
    QString line = QString(__FILE__) + "::" + QString::number(__LINE__) + ": ";
    return res == 0 ? QString::number(GetLastError()) : line + buffer;
}

class WindowsWatcherPrivate : public DirectoryWatcherPrivate {
    Q_OBJECT                                // 添加 Q_OBJECT 宏，启用信号槽机制
    Q_DECLARE_PUBLIC(WindowsWatcher)
    
public:
    explicit WindowsWatcherPrivate(WindowsWatcher* qq);
    
    HANDLE requestDirectoryHandle(const QString& path);

public slots:                               // 将 dispatchNotify 声明为槽函数
    void dispatchNotify(const QString& fileName, DWORD action);

private:
    QString oldFileName;                    // 用于重命名时暂存旧文件名
};

#endif // WINDOWSWATCHER_P_H
