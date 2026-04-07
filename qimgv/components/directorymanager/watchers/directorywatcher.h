// directorywatcher.h
#pragma once

#include <QObject>
#include <QString>

class DirectoryWatcherPrivate;

class DirectoryWatcher : public QObject {
    Q_OBJECT
public:
    static DirectoryWatcher* newInstance();
    virtual ~DirectoryWatcher();

    virtual void setWatchPath(const QString& watchPath);
    virtual QString watchPath() const;
    bool isObserving() const;

public Q_SLOTS:
    void observe();
    void stopObserving();
    void requestWatchPath(const QString& path);

Q_SIGNALS:
    void fileCreated(const QString& filePath);
    void fileDeleted(const QString& filePath);
    void fileRenamed(const QString& oldPath, const QString& newPath);
    void fileModified(const QString& filePath);
    void observingStarted();
    void observingStopped();

protected:
    explicit DirectoryWatcher(DirectoryWatcherPrivate* ptr);
    DirectoryWatcherPrivate* d_ptr;

private:
    Q_DECLARE_PRIVATE(DirectoryWatcher)
    Q_DISABLE_COPY_MOVE(DirectoryWatcher)
};