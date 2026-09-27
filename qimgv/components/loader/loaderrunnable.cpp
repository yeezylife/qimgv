#include "loaderrunnable.h"
#include "loader.h"
#include "utils/imagefactory.h"
#include <QMetaObject>
#include <utility>

LoaderRunnable::LoaderRunnable(Loader *loader, const QString &path,
                                 int allocationLimitMB, bool jxlAnimation, bool videoPlayback)
    : loader(loader), path(path),
      allocationLimitMB(allocationLimitMB), jxlAnimation(jxlAnimation), videoPlayback(videoPlayback) {}

void LoaderRunnable::run() {
    if (tryStart()) {
        // 赢得启动权：正常解码并投递结果（设置来自构造时的 GUI 线程快照）
        auto image = ImageFactory::createImage(path, allocationLimitMB, jxlAnimation, videoPlayback);
        // 结果经排队连接送回主线程；keepAlive 捕获在事件处理期间维系任务对象存活（无需 deleteLater）
        QMetaObject::invokeMethod(loader,
            [host = loader, p = std::move(path), image = std::move(image), keepAlive = self]() {
                (void)keepAlive;
                host->onLoadFinished(image, p);
            }, Qt::QueuedConnection);
    } else {
        // 已被 clearTasks 取消（CAS 保证与启动互斥，这里绝不会发生无效解码）：
        // 投递只持 keepAlive 的空事件，把最后引用的释放放到主线程，避免在 run() 内自毁
        QMetaObject::invokeMethod(loader,
            [keepAlive = self]() { (void)keepAlive; }, Qt::QueuedConnection);
    }
    // 释放本线程持有的引用；对象由事件 keepAlive 维系到主线程处理完
    self.reset();
}
