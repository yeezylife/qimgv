#include "windowswatcher_p.h"
#include "windowsworker.h"

WindowsWatcherPrivate::WindowsWatcherPrivate(WindowsWatcher* qq)
    : DirectoryWatcherPrivate(static_cast<DirectoryWatcher*>(qq), new WindowsWorker())
{
    auto windowsWorker = static_cast<WindowsWorker*>(worker.data());
    qRegisterMetaType<DWORD>("DWORD");
    connect(windowsWorker, &WindowsWorker::notifyEvent, this, &WindowsWatcherPrivate::dispatchNotify);
}

HANDLE WindowsWatcherPrivate::requestDirectoryHandle(const QString& path)
{
    HANDLE hDirectory;
    do {
        // 优化：直接使用 QString::utf16() 避免了 toStdWString() 产生的 std::wstring 临时对象内存分配和拷贝
        hDirectory = CreateFileW(
            reinterpret_cast<LPCWSTR>(path.utf16()),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr);

        if (hDirectory == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_SHARING_VIOLATION) {
                // 优化：缩短等待时间，提高 UI 响应速度（从 1000ms 减少到 100ms）
                QThread::msleep(100);
            } else {
                qDebug() << lastError();
                return INVALID_HANDLE_VALUE;
            }
        }
    } while (hDirectory == INVALID_HANDLE_VALUE);

    return hDirectory;
}

void WindowsWatcherPrivate::dispatchNotify(const QString& fileName, DWORD action)
{
    Q_Q(WindowsWatcher);

    if (fileName.isEmpty() || fileName.length() > 4096) {
        return;
    }

    switch (action) {
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
        break;
    }
}

WindowsWatcher::WindowsWatcher(QObject* parent)
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

void WindowsWatcher::setWatchPath(const QString &path)
{
    Q_D(WindowsWatcher);
    DirectoryWatcher::setWatchPath(path);

    HANDLE hDirectory = d->requestDirectoryHandle(path);
    if (hDirectory == INVALID_HANDLE_VALUE) {
        qDebug() << "requestDirectoryHandle: INVALID_HANDLE_VALUE";
        return;
    }

    auto windowsWorker = static_cast<WindowsWorker*>(d->worker.data());
    windowsWorker->setDirectoryHandle(hDirectory);
}