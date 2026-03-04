#include "windowswatcher_p.h"
#include "windowsworker.h"

WindowsWatcherPrivate::WindowsWatcherPrivate(WindowsWatcher* qq)
    : DirectoryWatcherPrivate(qq, new WindowsWorker())
{
    auto windowsWorker = static_cast<WindowsWorker*>(worker.data());
    // 移除指针类型注册，改用字符串传递
    // qRegisterMetaType<PFILE_NOTIFY_INFORMATION>("PFILE_NOTIFY_INFORMATION");

    // 新增：注册 DWORD 类型，确保信号槽能识别
    qRegisterMetaType<DWORD>("DWORD"); 

    // 修改：连接信号传递 QString 和 DWORD
    connect(windowsWorker, SIGNAL(notifyEvent(QString, DWORD)),
            this, SLOT(dispatchNotify(QString, DWORD)));
}

HANDLE WindowsWatcherPrivate::requestDirectoryHandle(const QString& path)
{
    HANDLE hDirectory;

    do {
        hDirectory = CreateFileW(
                    path.toStdWString().c_str(),
                    FILE_LIST_DIRECTORY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS,
                    nullptr);

        if (hDirectory == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_SHARING_VIOLATION)
            {
                qDebug() << "ERROR_SHARING_VIOLATION waiting for 1 sec";
                QThread::sleep(1);
            }
            else
            {
                qDebug() << lastError();
                return INVALID_HANDLE_VALUE;
            }
        }
    } while (hDirectory == INVALID_HANDLE_VALUE);
    return hDirectory;
}

void WindowsWatcherPrivate::dispatchNotify(const QString& fileName, DWORD action) {
    Q_Q(WindowsWatcher);

    // 验证文件名有效性（安全检查）
    if (fileName.isEmpty() || fileName.length() > 4096) {
        qDebug() << "Invalid file name received, ignoring:" << fileName;
        return;
    }

    switch (action)
    {
        case FILE_ACTION_ADDED:
            emit q->fileCreated(fileName);
            break;

        case FILE_ACTION_MODIFIED:
            emit q->fileModified(fileName);
            break;

        case FILE_ACTION_REMOVED:
            emit q->fileDeleted(fileName);
            break;

        case FILE_ACTION_RENAMED_NEW_NAME:
            emit q->fileRenamed(oldFileName, fileName);
            break;

        case FILE_ACTION_RENAMED_OLD_NAME:
            oldFileName = fileName;
            break;

        default:
            qDebug() << "Unknown notify action:" << action << "for file:" << fileName;
    }
}

WindowsWatcher::WindowsWatcher()
    : DirectoryWatcher(new WindowsWatcherPrivate(this))
{
    Q_D(WindowsWatcher);
    connect(d->workerThread.data(), &QThread::started, d->worker.data(), &WatcherWorker::run);
    d->worker.data()->moveToThread(d->workerThread.data());

    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());

    connect(windowsWorker, &WindowsWorker::finished, d->workerThread.data(), &QThread::quit);

    connect(windowsWorker, &WindowsWorker::started, this, &WindowsWatcher::observingStarted);
    connect(windowsWorker, &WindowsWorker::finished, this, &WindowsWatcher::observingStopped);
}

WindowsWatcher::WindowsWatcher(const QString& path)
    : DirectoryWatcher(new WindowsWatcherPrivate(this))
{
    Q_D(WindowsWatcher);
    setWatchPath(path);
}

void WindowsWatcher::setWatchPath(const QString &path) {
    Q_D(WindowsWatcher);
    DirectoryWatcher::setWatchPath(path);
    HANDLE hDirectory = d->requestDirectoryHandle(path);
    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        qDebug() << "requestDirectoryHandle: INVALID_HANDLE_VALUE";
        return;
    }

    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    windowsWorker->setDirectoryHandle(hDirectory);
}
