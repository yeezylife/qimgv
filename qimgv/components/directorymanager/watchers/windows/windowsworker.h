// windowsworker.h
#ifndef WINDOWSWORKER_H
#define WINDOWSWORKER_H

#include "../watcherworker.h"
#include <windows.h>
#include <QString>
#include <utility>

// RAII 封装 Windows 句柄，明确所有权
class ScopedHandle {
public:
    ScopedHandle() noexcept = default;
    explicit ScopedHandle(HANDLE h) noexcept : handle_(h) {}
    ~ScopedHandle() noexcept { reset(); }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.release()) {}
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = other.release();
        }
        return *this;
    }

    void reset(HANDLE h = INVALID_HANDLE_VALUE) noexcept {
        if (handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
        handle_ = h;
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
    void setDirectoryHandle(ScopedHandle handle);

signals:
    void notifyEvent(const QString& fileName, DWORD action);
    void finished();
    void started();

private:
    ScopedHandle hDirectory;
    QByteArray buffer;
};

#endif // WINDOWSWORKER_H
