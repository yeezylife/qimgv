#include "loaderrunnable.h"
#include "loader.h"
#include "utils/imagefactory.h"
#include <QMetaObject>
#include <utility>

LoaderRunnable::LoaderRunnable(Loader *loader, const QString &path,
                                 int allocationLimitMB, bool jxlAnimation, bool videoPlayback,
                                 quint64 generation)
    : loader(loader), path(path),
      allocationLimitMB(allocationLimitMB), jxlAnimation(jxlAnimation), videoPlayback(videoPlayback),
      generation(generation) {}

void LoaderRunnable::run() {
    if (tryStart()) {
        // 赢得启动权：正常解码并投递结果（设置来自构造时的 GUI 线程快照）
        auto image = ImageFactory::createImage(path, allocationLimitMB, jxlAnimation, videoPlayback);
        // 结果经排队连接送回主线程；keepAlive 捕获在事件处理期间维系任务对象存活（无需 deleteLater）
        QMetaObject::invokeMethod(loader,
            [host = loader, p = std::move(path), image = std::move(image), gen = generation, keepAlive = self]() {
                (void)keepAlive;
                host->onLoadFinished(image, p, gen);
            }, Qt::QueuedConnection);
    } else {
        // 已被 clearTasks 取消：归置区持有引用保证对象存活，此处直接释放本线程引用，
        // 不再投递空 keepAlive 事件（快速滚动时每取消一任务一次主线程唤醒纯开销）
    }
    // 释放本线程持有的引用；正常任务由事件 keepAlive 维系到主线程处理完，
    // 取消任务由 Loader::mCancelledHolders 维系到下次 clearTasks 批量释放
    self.reset();
}
