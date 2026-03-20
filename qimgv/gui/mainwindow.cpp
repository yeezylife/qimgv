#include "mainwindow.h"

// TODO: nuke this and rewrite
MW::MW(QWidget *parent)
: FloatingWidgetContainer(parent)
{
    // 优化：移除 WA_TranslucentBackground，减少 DWM 合成开销
    // setAttribute(Qt::WA_TranslucentBackground, true);
    
    layout.setContentsMargins(0,0,0,0);
    layout.setSpacing(0);
    setMinimumSize(10,10);
    
    // do not steal focus when clicked
    // this is just a container. accept key events only
    // via passthrough from child widgets
    setFocusPolicy(Qt::NoFocus);
    this->setLayout(&layout);
    setWindowTitle(QCoreApplication::applicationName() + QStringLiteral(" ") +
                   QCoreApplication::applicationVersion());
    this->setMouseTracking(true);
    this->setAcceptDrops(true);
    this->setAccessibleName("mainwindow");
    
    windowGeometryChangeTimer.setSingleShot(true);
    windowGeometryChangeTimer.setInterval(30);
    
    setupUi();
    
    connect(settings, &Settings::settingsChanged, this, &MW::readSettings);
    connect(&windowGeometryChangeTimer, &QTimer::timeout, this, &MW::onWindowGeometryChanged);
    connect(this, &MW::fullscreenStateChanged, this, &MW::adaptToWindowState);
    
    readSettings();
    currentDisplay = settings->lastDisplay();
    maximized = settings->maximizedWindow();
    restoreWindowGeometry();
}

/*                                                             |--[ImageViewer]
*                        |--[DocumentWidget]--[ViewerWidget]--|
* [MW]--[CentralWidget]--|                                    |--[VideoPlayer]
*                        |--[FolderView]
*
*  (not counting floating widgets)
*  ViewerWidget exists for input handling reasons (correct overlay hover handling)
*/
void MW::setupUi() {
    viewerWidget.reset(new ViewerWidget(this));
    infoBarWindowed.reset(new InfoBarProxy(this));
    docWidget.reset(new DocumentWidget(viewerWidget, infoBarWindowed));
    folderView.reset(new FolderViewProxy(this));
    
    connect(folderView.get(), &FolderViewProxy::sortingSelected, this, &MW::sortingSelected);
    connect(folderView.get(), &FolderViewProxy::directorySelected, this, &MW::opened);
    connect(folderView.get(), &FolderViewProxy::copyUrlsRequested, this, &MW::copyUrlsRequested);
    connect(folderView.get(), &FolderViewProxy::moveUrlsRequested, this, &MW::moveUrlsRequested);
    connect(folderView.get(), &FolderViewProxy::showFoldersChanged, this, &MW::showFoldersChanged);
    
    centralWidget.reset(new CentralWidget(docWidget, folderView, this));
    layout.addWidget(centralWidget.get());
    
    controlsOverlay = new ControlsOverlay(docWidget.get());
    infoBarFullscreen = new FullscreenInfoOverlayProxy(viewerWidget.get());
    sidePanel = new SidePanel(this);
    layout.addWidget(sidePanel);
    
    imageInfoOverlay = new ImageInfoOverlayProxy(viewerWidget.get());
    floatingMessage = new FloatingMessageProxy(viewerWidget.get());
    
    connect(viewerWidget.get(), &ViewerWidget::scalingRequested, this, &MW::scalingRequested);
    connect(viewerWidget.get(), &ViewerWidget::draggedOut, this, qOverload<>(&MW::draggedOut));
    connect(viewerWidget.get(), &ViewerWidget::playbackFinished, this, &MW::playbackFinished);
    connect(viewerWidget.get(), &ViewerWidget::showScriptSettings, this, &MW::showScriptSettings);
    
    connect(this, &MW::zoomIn,        viewerWidget.get(), &ViewerWidget::zoomIn);
    connect(this, &MW::zoomOut,       viewerWidget.get(), &ViewerWidget::zoomOut);
    connect(this, &MW::zoomInCursor,  viewerWidget.get(), &ViewerWidget::zoomInCursor);
    connect(this, &MW::zoomOutCursor, viewerWidget.get(), &ViewerWidget::zoomOutCursor);
    connect(this, &MW::scrollUp,    viewerWidget.get(), &ViewerWidget::scrollUp);
    connect(this, &MW::scrollDown,  viewerWidget.get(), &ViewerWidget::scrollDown);
    connect(this, &MW::scrollLeft,  viewerWidget.get(), &ViewerWidget::scrollLeft);
    connect(this, &MW::scrollRight, viewerWidget.get(), &ViewerWidget::scrollRight);
    connect(this, &MW::pauseVideo,     viewerWidget.get(), &ViewerWidget::pauseResumePlayback);
    connect(this, &MW::stopPlayback,   viewerWidget.get(), &ViewerWidget::stopPlayback);
    connect(this, &MW::seekVideoForward, viewerWidget.get(), &ViewerWidget::seekForward);
    connect(this, &MW::seekVideoBackward,  viewerWidget.get(), &ViewerWidget::seekBackward);
    connect(this, &MW::frameStep,      viewerWidget.get(), &ViewerWidget::frameStep);
    connect(this, &MW::frameStepBack,  viewerWidget.get(), &ViewerWidget::frameStepBack);
    connect(this, &MW::toggleMute,  viewerWidget.get(), &ViewerWidget::toggleMute);
    connect(this, &MW::volumeUp,  viewerWidget.get(), &ViewerWidget::volumeUp);
    connect(this, &MW::volumeDown, viewerWidget.get(), &ViewerWidget::volumeDown);
    connect(this, &MW::toggleTransparencyGrid, viewerWidget.get(), &ViewerWidget::toggleTransparencyGrid);
    connect(this, &MW::setLoopPlayback,  viewerWidget.get(), &ViewerWidget::setLoopPlayback);
}

void MW::setupFullUi() {
    if (fullUiInitialized) [[likely]] return;  // C++20: 热点分支优化
    fullUiInitialized = true;
    setupCropPanel();
    docWidget->allowPanelInit();
    docWidget->setupMainPanel();
    infoBarWindowed->init();
    infoBarFullscreen->init();
}

void MW::setupCropPanel() {
    if(cropPanel) [[likely]]
        return;
    cropOverlay = new CropOverlay(viewerWidget.get());
    cropPanel = new CropPanel(cropOverlay, this);
    connect(cropPanel, &CropPanel::cancel, this, &MW::hideCropPanel);
    connect(cropPanel, &CropPanel::crop,   this, &MW::hideCropPanel);
    connect(cropPanel, &CropPanel::crop,   this, &MW::cropRequested);
    connect(cropPanel, &CropPanel::cropAndSave, this, &MW::hideCropPanel);
    connect(cropPanel, &CropPanel::cropAndSave, this, &MW::cropAndSaveRequested);
}

void MW::setupCopyOverlay() {
    copyOverlay = new CopyOverlay(viewerWidget.get());
    connect(copyOverlay, &CopyOverlay::copyRequested, this, &MW::copyRequested);
    connect(copyOverlay, &CopyOverlay::moveRequested, this, &MW::moveRequested);
}

void MW::setupSaveOverlay() {
    saveOverlay = new SaveConfirmOverlay(viewerWidget.get());
    connect(saveOverlay, &SaveConfirmOverlay::saveClicked,    this, &MW::saveRequested);
    connect(saveOverlay, &SaveConfirmOverlay::saveAsClicked,  this, &MW::saveAsClicked);
    connect(saveOverlay, &SaveConfirmOverlay::discardClicked, this, &MW::discardEditsRequested);
}

void MW::setupRenameOverlay() {
    renameOverlay = new RenameOverlay(this);
    renameOverlay->setName(info.fileName);
    connect(renameOverlay, &RenameOverlay::renameRequested, this, &MW::renameRequested);
}

void MW::toggleFolderView() {
    hideCropPanel();
    if(copyOverlay)
        copyOverlay->hide();
    if(renameOverlay)
        renameOverlay->hide();
    docWidget->hideFloatingPanel();
    imageInfoOverlay->hide();
    centralWidget->toggleViewMode();
    onInfoUpdated();
}

void MW::enableFolderView() {
    hideCropPanel();
    if(copyOverlay)
        copyOverlay->hide();
    if(renameOverlay)
        renameOverlay->hide();
    docWidget->hideFloatingPanel();
    imageInfoOverlay->hide();
    centralWidget->showFolderView();
    onInfoUpdated();
}

void MW::enableDocumentView() {
    setupFullUi();
    centralWidget->showDocumentView();
    onInfoUpdated();
}

ViewMode MW::currentViewMode() {
    return centralWidget->currentViewMode();
}

void MW::fitWindow() {
    if(viewerWidget->interactionEnabled()) {
        viewerWidget->fitWindow();
    } else {
        showMessage("Zoom temporary disabled");
    }
}

void MW::fitWidth() {
    if(viewerWidget->interactionEnabled()) {
        viewerWidget->fitWidth();
    } else {
        showMessage("Zoom temporary disabled");
    }
}

void MW::fitOriginal() {
    if(viewerWidget->interactionEnabled()) {
        viewerWidget->fitOriginal();
    } else {
        showMessage("Zoom temporary disabled");
    }
}

void MW::fitWindowStretch() {
    if(viewerWidget->interactionEnabled()) {
        viewerWidget->fitWindowStretch();
    } else {
        showMessage("Zoom temporary disabled");
    }
}

void MW::switchFitMode() {
    if(viewerWidget->fitMode() == FIT_WINDOW)
        viewerWidget->setFitMode(FIT_ORIGINAL);
    else
        viewerWidget->setFitMode(FIT_WINDOW);
}

void MW::closeImage() {
    info.fileName.clear();
    info.filePath.clear();
    viewerWidget->closeImage();
}

void MW::preShowResize(QSize sz) {
    auto screens = qApp->screens();
    if(this->windowState() != Qt::WindowNoState || !screens.count() || screens.count() <= currentDisplay)
        return;
    
    int decorationSize = frameGeometry().height() - height();
    float maxSzMulti = static_cast<float>(settings->autoResizeLimit()) / 100.f;
    QRect availableGeom = screens.at(currentDisplay)->availableGeometry();
    QSize maxSz = availableGeom.size() * maxSzMulti;
    maxSz.setHeight(maxSz.height() - decorationSize);
    
    if(!sz.isEmpty()) {
        if(sz.width() > maxSz.width() || sz.height() > maxSz.height())
            sz.scale(maxSz, Qt::KeepAspectRatio);
    } else {
        sz = maxSz;
    }
    
    QRect newGeom(0, 0, sz.width(), sz.height());
    newGeom.moveCenter(availableGeom.center());
    newGeom.translate(0, decorationSize / 2);
    
    if(this->isVisible())
        setGeometry(newGeom);
    else
        settings->setWindowGeometry(newGeom);
    
    // 优化：移除 processEvents，依赖 Qt 自然事件循环
}

void MW::showImage(std::unique_ptr<QPixmap> pixmap) {
    if(settings->autoResizeWindow())
        preShowResize(pixmap->size());
    viewerWidget->showImage(std::move(pixmap));
    updateCropPanelData();
}

void MW::showAnimation(const std::shared_ptr<QMovie>& movie) {
    if(settings->autoResizeWindow())
        preShowResize(movie->frameRect().size());
    viewerWidget->showAnimation(movie);
    updateCropPanelData();
}

void MW::showVideo(QString&& file) {
    if(settings->autoResizeWindow())
        preShowResize(QSize());
    viewerWidget->showVideo(std::move(file));
}

void MW::showContextMenu() {
    viewerWidget->showContextMenu();
}

void MW::onSortingChanged(SortingMode mode) {
    folderView.get()->onSortingChanged(mode);
    if(centralWidget.get()->currentViewMode() == ViewMode::MODE_DOCUMENT) {
        switch(mode) {
            case SortingMode::SORT_NAME:      showMessage("Sorting: By Name");              break;
            case SortingMode::SORT_NAME_DESC: showMessage("Sorting: By Name (desc.)");      break;
            case SortingMode::SORT_TIME:      showMessage("Sorting: By Time");              break;
            case SortingMode::SORT_TIME_DESC: showMessage("Sorting: By Time (desc.)");      break;
            case SortingMode::SORT_SIZE:      showMessage("Sorting: By File Size");         break;
            case SortingMode::SORT_SIZE_DESC: showMessage("Sorting: By File Size (desc.)"); break;
        }
    }
}

void MW::setDirectoryPath(const QString& path) {
    info.directoryPath = path;
    info.directoryName = path.split(u'/').last();
    folderView->setDirectoryPath(path);
    onInfoUpdated();
}

void MW::toggleLockZoom() {
    viewerWidget->toggleLockZoom();
    if(viewerWidget->lockZoomEnabled())
        showMessage("Zoom lock: ON");
    else
        showMessage("Zoom lock: OFF");
    onInfoUpdated();
}

void MW::toggleLockView() {
    viewerWidget->toggleLockView();
    if(viewerWidget->lockViewEnabled())
        showMessage("View lock: ON");
    else
        showMessage("View lock: OFF");
    onInfoUpdated();
}

void MW::toggleFullscreenInfoBar() {
    if(!this->isFullScreen())
        return;
    showInfoBarFullscreen = !showInfoBarFullscreen;
    if(showInfoBarFullscreen)
        infoBarFullscreen->showWhenReady();
    else
        infoBarFullscreen->hide();
}

void MW::toggleImageInfoOverlay() {
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;
    if(imageInfoOverlay->isHidden())
        imageInfoOverlay->show();
    else
        imageInfoOverlay->hide();
}

void MW::toggleRenameOverlay(const QString& currentName) {
    if(!renameOverlay)
        setupRenameOverlay();
    if(renameOverlay->isHidden()) {
        renameOverlay->setBackdropEnabled((centralWidget->currentViewMode() == MODE_FOLDERVIEW));
        renameOverlay->setName(currentName);
        renameOverlay->show();
    } else {
        renameOverlay->hide();
    }
}

void MW::toggleScalingFilter() {
    ScalingFilter configuredFilter = settings->scalingFilter();
    if(viewerWidget->scalingFilter() == configuredFilter) {
        setFilterNearest();
    } else {
        setFilter(configuredFilter);
    }
}

void MW::setFilterNearest() {
    showMessage("Filter: nearest", 600);
    viewerWidget->setFilterNearest();
}

void MW::setFilterBilinear() {
    showMessage("Filter: bilinear", 600);
    viewerWidget->setFilterBilinear();
}

void MW::setFilter(ScalingFilter filter) {
    QString filterName;
    switch (filter) {
        case QI_FILTER_NEAREST:
            filterName = u"nearest";
            break;
        case ScalingFilter::QI_FILTER_BILINEAR:
            filterName = u"bilinear";
            break;
        case QI_FILTER_CV_BILINEAR_SHARPEN:
            filterName = u"bilinear + sharpen";
            break;
        case QI_FILTER_CV_CUBIC:
            filterName = u"bicubic";
            break;
        case QI_FILTER_CV_CUBIC_SHARPEN:
            filterName = u"bicubic + sharpen";
            break;
        default:
            filterName = u"configured " + QString::number(static_cast<int>(filter));
            break;
    }
    showMessage(u"Filter %1"_qs.arg(filterName), 600);
    viewerWidget->setScalingFilter(filter);
}

bool MW::isCropPanelActive() {
    return (activeSidePanel == SIDEPANEL_CROP);
}

void MW::onScalingFinished(const QPixmap& scaled) {
    viewerWidget->onScalingFinished(scaled);
}

void MW::saveWindowGeometry() {
    if(this->windowState() == Qt::WindowNoState)
        settings->setWindowGeometry(geometry());
    settings->setMaximizedWindow(maximized);
}

void MW::restoreWindowGeometry() {
    this->setGeometry(settings->windowGeometry());
    if(settings->maximizedWindow())
        this->setWindowState(Qt::WindowMaximized);
    updateCurrentDisplay();
}

void MW::updateCurrentDisplay() {
    auto screens = qApp->screens();
    currentDisplay = static_cast<int>(screens.indexOf(this->window()->screen()));
}

void MW::onWindowGeometryChanged() {
    saveWindowGeometry();
    updateCurrentDisplay();
}

void MW::saveCurrentDisplay() {
    settings->setLastDisplay(qApp->screens().indexOf(this->window()->screen()));
}

void MW::showEvent(QShowEvent *event) {
    FloatingWidgetContainer::showEvent(event);
    if (!firstShowHandled) {
        firstShowHandled = true;
        QTimer::singleShot(0, this, [this] {
            this->activateWindow();
            if (viewerWidget && viewerWidget->isVisible()) {
                viewerWidget->setFocus(Qt::OtherFocusReason);
            }
        });
    }
}

void MW::mouseMoveEvent(QMouseEvent *event) {
    event->ignore();
}

bool MW::event(QEvent *event) {
    if(event->type() == QEvent::WindowStateChange && this->isVisible() && !this->isFullScreen())
        maximized = isMaximized();
    if(event->type() == QEvent::Move || event->type() == QEvent::Resize)
        windowGeometryChangeTimer.start();
    return QWidget::event(event);
}

void MW::keyPressEvent(QKeyEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::wheelEvent(QWheelEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::mousePressEvent(QMouseEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::mouseReleaseEvent(QMouseEvent *event) {
    event->accept();
    actionManager->processEvent(event);
}

void MW::mouseDoubleClickEvent(QMouseEvent *event) {
    event->accept();
    QMouseEvent fakePressEvent(
        QEvent::MouseButtonPress,
        event->position(),
        event->scenePosition(),
        event->globalPosition(),
        event->button(),
        event->buttons(),
        event->modifiers(),
        event->source()
    );
    actionManager->processEvent(&fakePressEvent);
    actionManager->processEvent(event);
}

void MW::close() {
    saveWindowGeometry();
    saveCurrentDisplay();
    if(copyOverlay)
        copyOverlay->saveSettings();
    QWidget::close();
}

void MW::closeEvent(QCloseEvent *event) {
    event->accept();
    actionManager->invokeAction("exit");
}

void MW::dragEnterEvent(QDragEnterEvent *e) {
    if(e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MW::dropEvent(QDropEvent *event) {
    emit droppedIn(event->mimeData(), event->source());
}

void MW::resizeEvent(QResizeEvent *event) {
    if(activeSidePanel == SIDEPANEL_CROP) {
        cropOverlay->setImageScale(viewerWidget->currentScale());
        cropOverlay->setImageDrawRect(viewerWidget->imageRect());
    }
    FloatingWidgetContainer::resizeEvent(event);
}

void MW::showDefault() {
    if(!this->isVisible()) {
        if(settings->fullscreenMode())
            showFullScreen();
        else
            showWindowed();
    }
}

void MW::showSaveDialog(const QString& filePath) {
    QString newFilePath = getSaveFileName(filePath);
    if(!newFilePath.isEmpty())
        emit saveAsRequested(newFilePath);
}

QString MW::getSaveFileName(const QString & filePath) {
    docWidget->hideFloatingPanel();
    QStringList filters;
    filters.reserve(20);  // 优化：预留空间
    
    const auto writerFormats = QImageWriter::supportedImageFormats();
    
    auto addFilter = [&](const QByteArray& format, const QString& description) {
        if(writerFormats.contains(format)) {
            filters.append(description);
        }
    };
    
    addFilter("jpg",  u"JPEG (*.jpg *.jpeg *jpe *jfif)"_qs);
    addFilter("png",  u"PNG (*.png)"_qs);
    addFilter("webp", u"WebP (*.webp)"_qs);
    addFilter("jp2",  u"JPEG 2000 (*.jp2 *.j2k *.jpf *.jpx *.jpm *.jpgx)"_qs);
    addFilter("jxl",  u"JPEG-XL (*.jxl)"_qs);
    addFilter("avif", u"AVIF (*.avif *.avifs)"_qs);
    addFilter("tif",  u"TIFF (*.tif *.tiff)"_qs);
    addFilter("bmp",  u"BMP (*.bmp)"_qs);
#ifdef _WIN32
    addFilter("ico",  u"Icon Files (*.ico)"_qs);
#endif
    addFilter("ppm",  u"PPM (*.ppm)"_qs);
    addFilter("xbm",  u"XBM (*.xbm)"_qs);
    addFilter("xpm",  u"XPM (*.xpm)"_qs);
    addFilter("dds",  u"DDS (*.dds)"_qs);
    addFilter("wbmp", u"WBMP (*.wbmp)"_qs);
    
    for (const auto& format : writerFormats) {
        QString fmtStr = QString::fromUtf8(format);
        if (filters.join(u' ').contains(fmtStr))
            continue;
        filters.append(fmtStr.toUpper() + u" (*." + fmtStr + u")");
    }
    
    QString filterString = filters.join(u";; "_qs);
    QString selectedFilter = u"JPEG (*.jpg *.jpeg *jpe *jfif)"_qs;
    
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    for(const auto& filter : filters) {
        if(filter.contains(suffix)) {
            selectedFilter = filter;
            break;
        }
    }
    
    QString newFilePath = QFileDialog::getSaveFileName(this, tr("Save File as..."), filePath, filterString, &selectedFilter);
    return newFilePath;
}

void MW::showOpenDialog(const QString& path) {
    docWidget->hideFloatingPanel();
    QFileDialog dialog(this);
    QStringList imageFilter;
    imageFilter.append(settings->supportedFormatsFilter());
    imageFilter.append(u"All Files (*)"_qs);
    dialog.setDirectory(path);
    dialog.setNameFilters(imageFilter);
    dialog.setWindowTitle(u"Open image"_qs);
    dialog.setWindowModality(Qt::ApplicationModal);
    connect(&dialog, &QFileDialog::fileSelected, this, &MW::opened);
    dialog.exec();
}

void MW::showResizeDialog(QSize initialSize) {
    ResizeDialog dialog(initialSize, this);
    connect(&dialog, &ResizeDialog::sizeSelected, this, &MW::resizeRequested);
    dialog.exec();
}

DialogResult MW::fileReplaceDialog(const QString &source, const QString &target, FileReplaceMode mode, bool multiple) {
    FileReplaceDialog dialog(this);
    dialog.setModal(true);
    dialog.setSource(source);
    dialog.setDestination(target);
    dialog.setMode(mode);
    dialog.setMulti(multiple);
    dialog.exec();
    return dialog.getResult();
}

void MW::showSettings() {
    docWidget->hideFloatingPanel();
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

void MW::showScriptSettings() {
    docWidget->hideFloatingPanel();
    SettingsDialog settingsDialog(this);
    settingsDialog.switchToPage(4);
    settingsDialog.exec();
}

void MW::triggerFullScreen() {
    if(!isFullScreen()) {
        showFullScreen();
    } else {
        showWindowed();
    }
}

void MW::showFullScreen() {
    if(!isHidden())
        saveWindowGeometry();
    
    auto screens = qApp->screens();
    int _currentDisplay = static_cast<int>(screens.indexOf(this->window()->screen()));
    
    if(screens.count() > currentDisplay && currentDisplay != _currentDisplay) {
        this->move(screens.at(currentDisplay)->geometry().x(),
                   screens.at(currentDisplay)->geometry().y());
    }
    
    QWidget::showFullScreen();
    // 优化：移除 processEvents，依赖 Qt 自然事件循环
    emit fullscreenStateChanged(true);
}

void MW::showWindowed() {
    if(isFullScreen())
        QWidget::showNormal();
    restoreWindowGeometry();
    QWidget::show();
    // 优化：移除 processEvents，依赖 Qt 自然事件循环
    emit fullscreenStateChanged(false);
}

void MW::updateCropPanelData() {
    if(cropPanel && activeSidePanel == SIDEPANEL_CROP) {
        cropPanel->setImageRealSize(viewerWidget->sourceSize());
        cropOverlay->setImageDrawRect(viewerWidget->imageRect());
        cropOverlay->setImageScale(viewerWidget->currentScale());
        cropOverlay->setImageRealSize(viewerWidget->sourceSize());
    }
}

void MW::showSaveOverlay() {
    if(!settings->showSaveOverlay())
        return;
    if(!saveOverlay)
        setupSaveOverlay();
    saveOverlay->show();
}

void MW::hideSaveOverlay() {
    if(!saveOverlay)
        return;
    saveOverlay->hide();
}

void MW::showChangelogWindow() {
    changelogWindow->show();
}

void MW::showChangelogWindow(const QString &text) {
    changelogWindow->setText(text);
    changelogWindow->show();
}

void MW::triggerCropPanel() {
    if(activeSidePanel != SIDEPANEL_CROP) {
        showCropPanel();
    } else {
        hideCropPanel();
    }
}

void MW::showCropPanel() {
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;
    if(activeSidePanel != SIDEPANEL_CROP) {
        docWidget->hideFloatingPanel();
        sidePanel->setWidget(cropPanel);
        sidePanel->show();
        cropOverlay->show();
        activeSidePanel = SIDEPANEL_CROP;
        viewerWidget->fitWindow();
        setInteractionEnabled(false);
        updateCropPanelData();
    }
}

void MW::setInteractionEnabled(bool mode) {
    docWidget->setInteractionEnabled(mode);
    viewerWidget->setInteractionEnabled(mode);
}

void MW::hideCropPanel() {
    sidePanel->hide();
    if(activeSidePanel == SIDEPANEL_CROP) {
        cropOverlay->hide();
        setInteractionEnabled(true);
    }
    activeSidePanel = SIDEPANEL_NONE;
}

void MW::triggerCopyOverlay() {
    if(!viewerWidget->isDisplaying())
        return;
    if(!copyOverlay)
        setupCopyOverlay();
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;
    if(copyOverlay->operationMode() == OVERLAY_COPY) {
        copyOverlay->isHidden() ? copyOverlay->show() : copyOverlay->hide();
    } else {
        copyOverlay->setDialogMode(OVERLAY_COPY);
        copyOverlay->show();
    }
}

void MW::triggerMoveOverlay() {
    if(!viewerWidget->isDisplaying())
        return;
    if(!copyOverlay)
        setupCopyOverlay();
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW)
        return;
    if(copyOverlay->operationMode() == OVERLAY_MOVE) {
        copyOverlay->isHidden() ? copyOverlay->show() : copyOverlay->hide();
    } else {
        copyOverlay->setDialogMode(OVERLAY_MOVE);
        copyOverlay->show();
    }
}

void MW::closeFullScreenOrExit() {
    if(this->isFullScreen()) {
        this->showWindowed();
    } else {
        actionManager->invokeAction("exit");
    }
}

void MW::setCurrentInfo(int _index, int _fileCount, const QString& _filePath, const QString& _fileName, QSize _imageSize, qint64 _fileSize, bool slideshow, bool shuffle, bool edited) {
    info.index = _index;
    info.fileCount = _fileCount;
    info.fileName = _fileName;
    info.filePath = _filePath;
    info.imageSize = _imageSize;
    info.fileSize = _fileSize;
    info.slideshow = slideshow;
    info.shuffle = shuffle;
    info.edited = edited;
    onInfoUpdated();
}

QString MW::calculateWindowTitle() {
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW) {
        return tr("Folder view");
    }
    
    if(info.fileName.isEmpty()) {
        return qApp->applicationName();
    }
    
    // 优化：使用 arg 代替 + 拼接，减少临时字符串创建
    QString windowTitle = info.fileName;
    
    if(settings->windowTitleExtendedInfo()) {
        QString posString;
        if(info.fileCount)
            posString = u"[ %1/%2 ]"_qs.arg(info.index + 1).arg(info.fileCount);
        
        QString resString;
        if(info.imageSize.width())
            resString = u"%1 x %2"_qs.arg(info.imageSize.width()).arg(info.imageSize.height());
        
        QString sizeString;
        if(info.fileSize)
            sizeString = locale().formattedDataSize(info.fileSize, 1);
        
        if(!posString.isEmpty())
            windowTitle.prepend(posString + u"  "_qs);
        if(!resString.isEmpty())
            windowTitle.append(u"  -  "_qs + resString);
        if(!sizeString.isEmpty())
            windowTitle.append(u"  -  "_qs + sizeString);
    }
    
    QString states;
    if(info.slideshow)
        states.append(u" [slideshow]"_qs);
    if(info.shuffle)
        states.append(u" [shuffle]"_qs);
    if(viewerWidget->lockZoomEnabled())
        states.append(u" [zoom lock]"_qs);
    if(viewerWidget->lockViewEnabled())
        states.append(u" [view lock]"_qs);
    
    if(!settings->infoBarWindowed() && !states.isEmpty())
        windowTitle.append(u" -"_qs + states);
    
    if(info.edited)
        windowTitle.prepend(u"* "_qs);
    
    return windowTitle;
}

void MW::calculateInfoBarContent(QString& infoText, QString& sizeText) {
    if(centralWidget->currentViewMode() == MODE_FOLDERVIEW || info.fileName.isEmpty()) {
        infoText = tr("No file opened.");
        sizeText.clear();
        return;
    }
    
    // 优化：使用 arg 代替 + 拼接
    infoText = info.fileName + (info.edited ? u"  *"_qs : QString());
    
    QString resString;
    if(info.imageSize.width())
        resString = u"%1 x %2"_qs.arg(info.imageSize.width()).arg(info.imageSize.height());
    
    QString sizeString;
    if(info.fileSize)
        sizeString = locale().formattedDataSize(info.fileSize, 1);
    
    sizeText = resString + u"  "_qs + sizeString;
    
    QString states;
    if(info.slideshow)
        states.append(u" [slideshow]"_qs);
    if(info.shuffle)
        states.append(u" [shuffle]"_qs);
    if(viewerWidget->lockZoomEnabled())
        states.append(u" [zoom lock]"_qs);
    if(viewerWidget->lockViewEnabled())
        states.append(u" [view lock]"_qs);
    
    if(!settings->infoBarWindowed() && !states.isEmpty())
        sizeText.append(u" "_qs + states);
}

void MW::onInfoUpdated() {
    if(renameOverlay)
        renameOverlay->setName(info.fileName);
    
    setWindowTitle(calculateWindowTitle());
    
    QString infoText, sizeText;
    calculateInfoBarContent(infoText, sizeText);
    
    QString posString;
    if(info.fileCount)
        posString = u"[ %1/%2 ]"_qs.arg(info.index + 1).arg(info.fileCount);
    
    infoBarFullscreen->setInfo(posString, infoText, sizeText);
    infoBarWindowed->setInfo(posString, infoText, sizeText);
}

void MW::setExifInfo(const QHash<QString, QString> &info) {
    m_exifInfo = info;
    if(imageInfoOverlay)
        imageInfoOverlay->setExifInfo(info);
}

std::shared_ptr<FolderViewProxy> MW::getFolderView() {
    return folderView;
}

std::shared_ptr<ThumbnailStripProxy> MW::getThumbnailPanel() {
    return docWidget->thumbPanel();
}

void MW::showMessageDirectory(const QString& dirName) {
    floatingMessage->showMessage(dirName, FloatingMessageIcon::ICON_DIRECTORY, 1700);
}

void MW::showMessageDirectoryEnd() {
    // TODO replace with something nicer
}

void MW::showMessageDirectoryStart() {
    // TODO replace with something nicer
}

void MW::showMessageFitWindow() {
    floatingMessage->showMessage(tr("Fit Window"), FloatingMessageIcon::NO_ICON, 350);
}

void MW::showMessageFitWidth() {
    floatingMessage->showMessage(tr("Fit Width"), FloatingMessageIcon::NO_ICON, 350);
}

void MW::showMessageFitOriginal() {
    floatingMessage->showMessage(tr("Fit 1:1"), FloatingMessageIcon::NO_ICON, 350);
}

void MW::showMessage(const QString& text) {
    floatingMessage->showMessage(text, FloatingMessageIcon::NO_ICON, 1500);
}

void MW::showMessage(const QString& text, int duration) {
    floatingMessage->showMessage(text, FloatingMessageIcon::NO_ICON, duration);
}

void MW::showMessageSuccess(const QString& text) {
    floatingMessage->showMessage(text, FloatingMessageIcon::ICON_SUCCESS, 1500);
}

void MW::showWarning(const QString& text) {
    floatingMessage->showMessage(text, FloatingMessageIcon::ICON_WARNING, 1500);
}

void MW::showError(const QString& text) {
    floatingMessage->showMessage(text, FloatingMessageIcon::ICON_ERROR, 2800);
}

bool MW::showConfirmation(const QString& title, const QString& msg) {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(msg);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setModal(true);
    return msgBox.exec() == QMessageBox::Yes;
}

void MW::readSettings() {
    showInfoBarFullscreen = settings->infoBarFullscreen();
    showInfoBarWindowed = settings->infoBarWindowed();
    adaptToWindowState();
}

void MW::applyWindowedBackground() {
#ifdef USE_KDE_BLUR
    QWindow* window = this->windowHandle();
    if(window) {
        if(settings->backgroundOpacity() == 1.0)
            KWindowEffects::enableBlurBehind(window, false);
        else
            KWindowEffects::enableBlurBehind(window, settings->blurBackground());
    }
#endif
}

void MW::applyFullscreenBackground() {
#ifdef USE_KDE_BLUR
    QWindow* window = this->windowHandle();
    if(window)
        KWindowEffects::enableBlurBehind(window, false);
#endif
}

void MW::adaptToWindowState() {
    docWidget->hideFloatingPanel();
    if(isFullScreen()) {
        applyFullscreenBackground();
        infoBarWindowed->hide();
        if(showInfoBarFullscreen)
            infoBarFullscreen->showWhenReady();
        else
            infoBarFullscreen->hide();
        auto pos = settings->panelPosition();
        if(!settings->panelEnabled() || pos == PANEL_BOTTOM || pos == PANEL_LEFT)
            controlsOverlay->show();
        else
            controlsOverlay->hide();
    } else {
        applyWindowedBackground();
        infoBarFullscreen->hide();
        if(showInfoBarWindowed)
            infoBarWindowed->show();
        else
            infoBarWindowed->hide();
        controlsOverlay->hide();
    }
    folderView->onFullscreenModeChanged(isFullScreen());
    docWidget->onFullscreenModeChanged(isFullScreen());
    viewerWidget->onFullscreenModeChanged(isFullScreen());
}

void MW::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.fillRect(rect(), Qt::black);
    FloatingWidgetContainer::paintEvent(event);
}

void MW::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    docWidget->hideFloatingPanel(true);
}