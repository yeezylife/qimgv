#pragma once

#include <QRunnable>
#include <QString>
#include <atomic>
#include <cstdint>
#include <memory>

class Loader;
class Image;

// 普通 QRunnable，不继承 QObject：省去每任务 QObject 元对象与信号机制的分配开销。
// 结果经 QMetaObject::invokeMethod(排队连接) 送回主线程。
// 生命周期由 shared_ptr 管理：self 引用保证排队/运行期间对象存活，
// run() 末尾 self.reset() 释放本线程引用；正常任务由结果事件 keepAlive 维系到事件处理完，
// 被取消的任务由空事件 keepAlive 在主线程释放最后引用——均不在 run() 内自毁。
// 启动/取消由 tryStart/tryCancel 单次 CAS 仲裁（与 QThreadPool 是否已取走任务无关）：
// 两方只有一方能赢，从根上消除"已取消却仍解码"的边界浪费。
class LoaderRunnable : public QRunnable {
public:
    // 构造于 GUI 线程：解码限额与格式开关在此快照，run() 所在 worker 线程只读
    LoaderRunnable(Loader *loader, const QString &path,
                   int allocationLimitMB, bool jxlAnimation, bool videoPlayback);
    void run() override;

    // 单字原子状态：NotStarted → Running（run 赢得 CAS）或 Cancelled（clearTasks 赢得 CAS）。
    // 仅转移一次，无 ABA；单个 uint64_t 也避免多个 1 字节原子成员引入结构填充（-Wpadded 告警）
    bool tryStart() noexcept {
        std::uint64_t expected = NotStarted;
        return state.compare_exchange_strong(expected, Running);
    }
    bool tryCancel() noexcept {
        std::uint64_t expected = NotStarted;
        return state.compare_exchange_strong(expected, Cancelled);
    }

private:
    static constexpr std::uint64_t NotStarted = 0;
    static constexpr std::uint64_t Running = 1;
    static constexpr std::uint64_t Cancelled = 2;

    Loader *loader;
    QString path;
    // 主线程快照、worker 只读：int 在前 bool 紧随，填充最少
    int allocationLimitMB;
    bool jxlAnimation;
    bool videoPlayback;

public:
    std::shared_ptr<LoaderRunnable> self; // 由 doLoadAsync 赋值，run() 末尾释放
    std::atomic<std::uint64_t> state{NotStarted};
};
