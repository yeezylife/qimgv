// windowsworker.h
#ifndef WINDOWSWORKER_H
#define WINDOWSWORKER_H

#include "../watcherworker.h"
#include <windows.h>
#include <QString>
#include <QMutex>
#include <algorithm>
#include <utility>

class ScopedHandle {
public:
    ScopedHandle() noexcept = default;
    explicit ScopedHandle(HANDLE h) noexcept : handle_(h) {}
    ~ScopedHandle() noexcept { close(); }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.release()) {}
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            close();
            handle_ = other.release();
        }
        return *this;
    }

    void reset(HANDLE h = INVALID_HANDLE_VALUE) noexcept { close(); handle_ = h; }

    void close() noexcept {
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE get() const noexcept { return handle_; }
    HANDLE release() noexcept { return std::exchange(handle_, INVALID_HANDLE_VALUE); }
    explicit operator bool() const noexcept { return handle_ != INVALID_HANDLE_VALUE; }

private:
    HANDLE handle_ = INVALID_HANDLE_VALUE;
};

class WindowsWorker : public WatcherWorker {
    Q_OBJECT
public:
    explicit WindowsWorker();
    ~WindowsWorker() override = default;
    void run() override;
    void setRunning(bool running);
    void setWatchPath(const QString& path);
    // 这些方法可被任意线程直接调用，内部通过原子变量和互斥锁保证线程安全
    void requestDirectoryHandle(const QString& path);
    void cancelIo();

signals:
    void notifyEvent(const QString& fileName, DWORD action);
    void finished();
    void started();

private:
    HANDLE openDirectoryHandle(const QString& path);

    ScopedHandle hDirectory;
    QString watchPath;
    QString pendingPath;
    QByteArray buffer;
    std::atomic<bool> needsRestart{false};
    QMutex pathMutex;
};

#endif // WINDOWSWORKER_H