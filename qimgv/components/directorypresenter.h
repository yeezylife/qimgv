#pragma once

#include <QObject>
#include <memory>
#include "gui/idirectoryview.h"
#include "directorymodel.h"
#include "sharedresources.h"
#include <QMimeData>

//tmp
#include <QtSvg/QSvgRenderer>
#include <QPixmap>

class DirectoryPresenter : public QObject {
    Q_OBJECT
public:
    explicit DirectoryPresenter(QObject *parent = nullptr);

    void setView(const std::shared_ptr<IDirectoryView> &);
    void setModel(const std::shared_ptr<DirectoryModel> &newModel);
    void unsetModel();

    void selectAndFocus(int index);
    void selectAndFocus(const QString &path);

    void onFileRemoved(const QString &filePath, int index);
    void onFileRenamed(const QString &fromPath, int indexFrom, const QString &toPath, int indexTo);
    void onFileAdded(const QString &filePath);
    void onFileModified(const QString &filePath);

    void onDirRemoved(const QString &dirPath, int index);
    void onDirRenamed(const QString &fromPath, int indexFrom, const QString &toPath, int indexTo);
    void onDirAdded(const QString &dirPath);

    bool showDirs();
    void setShowDirs(bool mode);

    QList<QString> selectedPaths() const;


signals:
    void dirActivated(QString dirPath);
    void fileActivated(QString filePath);
    void draggedOut(QList<QString>);
    void droppedInto(QList<QString>, QString);

public slots:
    void disconnectView();
    void reloadModel();

private slots:
    void populateView();
    void onItemActivated(int absoluteIndex);
    void onDraggedOut();
    void onDraggedOver(int index);

    void onDroppedInto(const QMimeData *data, QObject *source, int targetIndex);
private:
    std::shared_ptr<IDirectoryView> view = nullptr;
    std::shared_ptr<DirectoryModel> model = nullptr;
    bool mShowDirs = false;
    QObject *viewObject = nullptr;
    
    // 目录图标缓存
    std::shared_ptr<const QPixmap> mCachedDirPixmap;
    int mCachedDirSize = 0;
    
    // 辅助函数：将文件索引转换为视图中的绝对索引
    int fileIndexToViewIndex(int fileIndex) const;
    
    // 辅助函数：获取 QObject 指针
    QObject *viewAsObject();
};
