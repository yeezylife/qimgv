#pragma once

#include <QObject>
#include <QDebug>
#include <QMutex>
#include <QClipboard>
#include <QDrag>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QTranslator>
#include "appversion.h"
#include "settings.h"
#include "components/directorymodel.h"
#include "components/scriptmanager/scriptmanager.h"
#include "gui/mainwindow.h"
#include "utils/randomizer.h"
#include "gui/dialogs/printdialog.h"

#ifdef __GLIBC__
#include <malloc.h>
#endif

struct State {
    bool hasActiveImage = false;
    bool delayModel = false;
    QString currentFilePath = "";
    QString directoryPath = "";
    std::shared_ptr<Image> currentImg;
    bool slideshowActive = false;
    bool shuffle = false;
    bool isEdited = false;
};

enum MimeDataTarget {
    TARGET_CLIPBOARD,
    TARGET_DROP
};

class Core : public QObject {
    Q_OBJECT
public:
    Core();
    void showGui();

signals:
    void firstImageReady();

public slots:
    void updateInfoString();
    bool loadPath(const QString& path);

private:
    QElapsedTimer t;

    void initGui();
    void initComponents();
    void connectComponents();
    void initActions();
    void loadTranslation();
    void onUpdate();
    void onFirstRun();

    // ui stuff
    std::unique_ptr<MW> mw;

    State state;
    bool loopSlideshow, shuffle, slideshow;
    FolderEndAction folderEndAction;

    // components
    std::shared_ptr<DirectoryModel> model;

    QString lastClipboardSaveFormat = "jxl";

    void rotateByDegrees(int degrees);
    void reset();
    bool setDirectory(const QString& path);

    QDrag *mDrag;
    QMimeData *getMimeDataForImage(const std::shared_ptr<Image>& img, MimeDataTarget target);
    QTranslator *translator = nullptr;

    Randomizer randomizer;
    void syncRandomizer();
    
    // 优化3：QDrag复用
    std::unique_ptr<QDrag> dragCache;
    

    void attachModel(std::unique_ptr<DirectoryModel> _model);
    QString selectedPath();
    void guiSetImage(const std::shared_ptr<Image>& img);
    QTimer slideshowTimer;

    void startSlideshowTimer();
    void startSlideshow();
    void stopSlideshow();

    bool saveFile(const QString &filePath, const QString &newPath);
    bool saveFile(const QString &filePath);

    std::shared_ptr<ImageStatic> getEditableImage(const QString &filePath);
    QList<QString> currentSelection();

    template<typename Func, typename... Args>
    void edit_template(bool save, const QString& action, Func editFunc, Args&&... as) {
        if(model->isEmpty())
            return;
        if(save && !mw->showConfirmation(action, tr("Perform action \"") + action + "\"? \n\n" + tr("Changes will be saved immediately.")))
            return;

        // 优化：预先提取选中路径，避免循环内重复调用
        const auto paths = currentSelection();
        for(const auto& path : paths) {
            auto img = getEditableImage(path);
            if(!img)
                continue;

            QImage result = editFunc(*img->getImage(), as...);
            
            // 优化：返回值安全移动，消除一次QImage拷贝
            img->setEditedImage(std::make_unique<QImage>(std::move(result)));
            
            model->updateImage(path, std::static_pointer_cast<Image>(img));
            if(save) {
                saveFile(path);
                if(state.currentFilePath != path)
                    model->unload(path);
            }
        }
        if(state.hasActiveImage) {
            auto img = model->getImage(state.currentFilePath);
            state.isEdited = img->isEdited();
        }
        updateInfoString();
    }

    void doInteractiveCopy(const QString& path, const QString& destDirectory, DialogResult &overwriteFiles);
    void doInteractiveMove(const QString& path, const QString& destDirectory, DialogResult &overwriteFiles);

private slots:
    void readSettings();
    void nextImage();
    void prevImage();
    void nextImageSlideshow();
    void jumpToFirst();
    void jumpToLast();
    void onModelItemReady(const std::shared_ptr<Image>& img, const QString& path);
    void onModelItemUpdated(const QString& fileName);
    void onModelSortingChanged(SortingMode mode);
    void onLoadFailed(const QString &path);
    void rotateLeft();
    void rotateRight();
    void close();
    void scalingRequest(QSize size, ScalingFilter filter);
    void onScalingFinished(const QPixmap& scaled, const ScalerRequest& req);
    void copyCurrentFile(const QString& destDirectory);
    void moveCurrentFile(const QString& destDirectory);
    void copyPathsTo(const QList<QString>& paths, const QString& destDirectory);
    void interactiveCopy(const QList<QString>& paths, const QString& destDirectory);
    void interactiveMove(const QList<QString>& paths, const QString& destDirectory);
    void movePathsTo(const QList<QString>& paths, const QString& destDirectory);
    FileOpResult removeFile(const QString& fileName, bool trash);
    void onFileRemoved(const QString& filePath, int index);
    void onFileRenamed(const QString& fromPath, int indexFrom, const QString& toPath, int indexTo);
    void onFileAdded(const QString& filePath);
    void onFileModified(const QString& filePath);
    void showResizeDialog();
    void resize(QSize size);
    void flipH();
    void flipV();
    void crop(QRect rect);
    void cropAndSave(QRect rect);
    void discardEdits();
    void toggleCropPanel();
    void toggleFullscreenInfoBar();
    void requestSavePath();
    void saveCurrentFile();
    void saveCurrentFileAs(const QString& destPath);
    void runScript(const QString& scriptName);
    void setWallpaper();
    void removePermanent();
    void moveToTrash();
    void reloadImage();
    void reloadImage(const QString& fileName);
    void copyFileClipboard();
    void copyPathClipboard();
    void openFromClipboard();
    void renameCurrentSelection(const QString& newName);
    void sortBy(SortingMode mode);
    void sortByName();
    void sortByTime();
    void sortBySize();
    void showRenameDialog();
    void onDraggedOut();
    void onDropIn(const QMimeData *mimeData, QObject* source);
    void toggleShuffle();
    void onModelLoaded();
    void outputError(const FileOpResult &error) const;
    void showOpenDialog();
    void showInDirectory();
    bool loadFileIndex(int index, bool async, bool preload);
    void enableDocumentView();
    void toggleSlideshow();
    void onPlaybackFinished();
    void loadParentDir();
    void nextDirectory();
    void prevDirectory(bool selectLast);
    void prevDirectory();
    void print();
    void modelDelayLoad();
};
