#pragma once

#include <QThreadPool>
#include <QHash>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include "loaderrunnable.h"

class Loader : public QObject {
    Q_OBJECT

    friend class LoaderRunnable;

public:
    explicit Loader();
    ~Loader();

    std::shared_ptr<Image> load(const QString &path);
    void loadAsyncPriority(const QString &path);
    void loadAsync(const QString &path);
    void clearTasks();
    
    bool isBusy() const;
    bool isLoading(const QString &path);

signals:
    void loadFinished(std::shared_ptr<Image>, const QString &path);
    void loadFailed(const QString &path);

private:
    QHash<QString, std::shared_ptr<LoaderRunnable>> tasks;
    QThreadPool *pool;         // 预加载线程池
    QThreadPool *priorityPool; // 当前图片专用，避免被运行中的 preload 卡住

    // ⭐ 翻页代际：priority 加载递增，过期 preload 结果直接丢弃，不进缓存
    std::atomic<quint64> mGeneration{0};
    QString mPriorityPath;
    // ⭐ 已取消任务的归置区：run() 取消分支不再逐任务投递空事件（N 次主线程唤醒），
    // 而是由归置区持有引用保证 run() 内自 reset 安全，单次批量释放
    std::vector<std::shared_ptr<LoaderRunnable>> mCancelledHolders;

    void doLoadAsync(QThreadPool *targetPool, const QString &path, quint64 generation);
    void onLoadFinished(const std::shared_ptr<Image> &image, const QString &path, quint64 generation);
};
