#pragma once

#include <QThreadPool>
#include <QHash>
#include <memory>
#include "loaderrunnable.h"

class Loader : public QObject {
    Q_OBJECT

public:
    explicit Loader();
    ~Loader();

    std::shared_ptr<Image> load(const QString &path);
    void loadAsyncPriority(const QString &path);
    void loadAsync(const QString &path);
    void clearTasks();
    
    bool isBusy() const;
    bool isLoading(const QString &path);

private:
    QHash<QString, LoaderRunnable*> tasks;
    QThreadPool *pool;

    void doLoadAsync(const QString &path, int priority);

signals:
    void loadFinished(std::shared_ptr<Image>, const QString &path);
    void loadFailed(const QString &path);

private slots:
    void onLoadFinished(const std::shared_ptr<Image> &image, const QString &path);
};
