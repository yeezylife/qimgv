#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QMutex>
#include <malloc.h>
#include "appversion.h"
#include "settings.h"
#include "components/directorymanager/directorymanager.h"
#include "components/loader/loader.h"
#include "components/scaler/scaler.h"
#include "components/thumbnailer/thumbnailer.h"
#include "components/scriptmanager/scriptmanager.h"
#include "gui/mainwindow.h"
#include "gui/viewers/viewerwidget.h"
#include "gui/viewers/imageviewer.h"

struct State {
    State() : currentIndex(0), hasActiveImage(false), isWaitingForLoader(false) {}
    int currentIndex;
    // TODO: come up with something better?
    QString displayingFileName;
    bool hasActiveImage, isWaitingForLoader;
};

class Core : public QObject {
    Q_OBJECT
public:
    Core();
    void showGui();

public slots:
    void updateInfoString();

    void loadByPath(QString filePath, bool blocking);

    // loads image in second thread
    void loadByPath(QString);

    // loads image in main thread
    void loadByPathBlocking(QString);

    // invalid position will be ignored
    bool loadByIndex(int index);
    bool loadByIndexBlocking(int index);

signals:
    void imageIndexChanged(int);

private:
    void initGui();
    void initComponents();
    void connectComponents();
    void initActions();
    void postUpdate();

    // ui stuff
    MainWindow *mw;
    ViewerWidget *viewerWidget;
    ThumbnailStrip *thumbnailPanelWidget;

    State state;
    QTimer *loadingTimer; // this is for loading message delay. TODO: replace with something better

    bool infiniteScrolling;

    // components
    Loader *loader;
    DirectoryManager *dirManager;
    Cache *cache;
    Scaler *scaler;
    Thumbnailer *thumbnailer;

    void rotateByDegrees(int degrees);
    void reset();
    bool setDirectory(QString newPath);
    void displayImage(Image *img);
    void loadDirectory(QString);
    void loadImage(QString path, bool blocking);
    void trimCache();
    void preload(int index);

private slots:
    void readSettings();
    void nextImage();
    void prevImage();
    void jumpToFirst();
    void jumpToLast();
    void onLoadFinished(std::shared_ptr<Image> img);
    void onLoadFailed(QString path);
    void onLoadStarted();
    void onLoadingTimeout();
    void clearCache();
    void stopPlayback();
    void rotateLeft();
    void rotateRight();
    void closeBackgroundTasks();
    void close();
    void switchFitMode();
    void scalingRequest(QSize);
    void onScalingFinished(QPixmap* scaled, ScalerRequest req);
    void forwardThumbnail(std::shared_ptr<Thumbnail> thumbnail);
    void removeFile();
    void moveFile(QString destDirectory);
    void copyFile(QString destDirectory);
    void removeFile(int index);
    void onFileRemoved(int index);
    void onFileAdded(int index);
    void showResizeDialog();
    void resize(QSize size);
    void flipH();
    void flipV();
    void crop(QRect rect);
    void discardEdits();
    void toggleCropPanel();
    void fitWindow();
    void fitWidth();
    void fitOriginal();
    void requestSavePath();
    void saveImageToDisk();
    void saveImageToDisk(QString);
    void runScript(const QString&);
};

#endif // CORE_H
