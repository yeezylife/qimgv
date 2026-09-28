// windowsworker.h
#ifndef WINDOWSWORKER_H
#define WINDOWSWORKER_H

#include "../watcherworker.h"
#include <windows.h>
#include <QString>
#include <QByteArray>
#include <QVector>
#include <QMetaType>
#include <QMutex>
#include <atomic>
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

// 一次 ReadDirectoryChangesW 返回的单个通知条目
struct NotifyEvent {
    QString fileName;
    DWORD action;
};
Q_DECLARE_METATYPE(NotifyEvent)

class WindowsWorker : public WatcherWorker {
    Q_OBJECT
public:
    explicit WindowsWorker();
    ~WindowsWorker() override = default;
    void run() override;
    void setRunning(bool running);
    void setWatchPath(const QString& path);
    // 这些方法可被任意线程直接调用，句柄通过原子变量暴露，路径经互斥锁保护
    void requestDirectoryHandle(const QString& path);
    void cancelIo();

signals:
    // ⭐ 批量派发，避免每个事件一个排队信号
    void notifyEvents(const QVector<NotifyEvent>& events);
    void finished();
    void started();

private:
    HANDLE openDirectoryHandle(const QString& path);
    // ⭐ run() 拆分：每 helper 单一职责，run 仅保留主循环编排
    bool ensureInitialHandle(QString &currentPath);
    // 返回 false 表示应退出主循环
    bool handleRestart(QString &currentPath, int &quietReads);
    enum class WaitResult { Ok, Stop, Retry };
    WaitResult waitOnce(DWORD &bytesReturned);
    QVector<NotifyEvent> parseBatch(DWORD bytesReturned);
    static void dedupModified(QVector<NotifyEvent> &batch);
    void emitAndMaybeShrink(const QVector<NotifyEvent> &batch, DWORD bytesReturned, int &quietReads);

    ScopedHandle hDirectory;
    std::atomic<HANDLE> activeHandle{INVALID_HANDLE_VALUE};
    QString pendingPath;
    QByteArray buffer;
    bool networkBufferCapped = false;   // ⭐ 网络目录(SMB)缓冲上限 64KB，仅 worker 线程访问
    std::atomic<bool> needsRestart{false};
    QMutex pathMutex;
};

#endif // WINDOWSWORKER_H
