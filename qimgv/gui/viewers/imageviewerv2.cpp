#include "imageviewerv2.h"
#include "sourcecontainers/imageanimated.h"
#include <QApplication>
#include <QPainter>
#include <QProcess>
#include <QPropertyAnimation>
#include <QScreen>
#include <ranges>
#include <algorithm>

ImageViewerV2::ImageViewerV2(QWidget* parent)
    : QGraphicsView(parent)
    , scene(nullptr)
    , pixmap(nullptr)
    , movie(nullptr)
    , animationTimer(nullptr)
    , scaleTimer(nullptr)
    , horizontalScroll(nullptr)
    , verticalScroll(nullptr)
    , scrollAnimationX(nullptr)
    , scrollAnimationY(nullptr)
    , mouseInteraction(MOUSE_NONE)
    , transparencyGrid(false)
    , expandImage(false)
    , keepFitMode(false)
    , loopPlayback(true)
    , mIsFullscreen(false)
    , scrollBarWorkaround(true)
    , useFixedZoomLevels(false)
    , trackpadDetection(true)
    , dragsEnabled(true)
    , wayland(false)
    , zoomStep(0.1f)
    , dpr(1.0f)
    , minScale(0.01f)
    , maxScale(500.0f)
    , fitWindowScale(0.125f)
    , fitWindowStretchScale(0.125f)
    , expandLimit(1.0f)
    , lockedScale(1.0f)
    , lastRequestedScale(-1.0f)
    , mViewLock(LOCK_NONE)
    , imageFitMode(FIT_WINDOW)
    , imageFitModeDefault(FIT_WINDOW)
    , zoomThreshold(ZOOM_THRESHOLD_FACTOR)
    , dragThreshold(10)
    , scaledImageFitsCache(true)
{
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAcceptDrops(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShape(QFrame::NoFrame);

    if (qApp->platformName() == "wayland")
        wayland = true;

    dpr = static_cast<float>(devicePixelRatioF());
    zoomThreshold = static_cast<int>(dpr * ZOOM_THRESHOLD_FACTOR);

    initializeScene();
    initializeTimers();
    initializeScrollBars();
    setupConnections();

    checkboard = QPixmap(":res/icons/common/other/checkerboard.png");
    lastTouchpadScroll.start();

    initSettings();  // replaced virtual call readSettings()
}

ImageViewerV2::~ImageViewerV2() = default;

void ImageViewerV2::initializeScene()
{
    pixmapItem.setTransformationMode(Qt::SmoothTransformation);
    pixmapItem.setScale(1.0f);
    pixmapItem.setOffset(CENTER_OFFSET, CENTER_OFFSET);
    pixmapItem.setTransformOriginPoint(CENTER_OFFSET, CENTER_OFFSET);

    pixmapItemScaled.setScale(1.0f);
    pixmapItemScaled.setOffset(CENTER_OFFSET, CENTER_OFFSET);
    pixmapItemScaled.setTransformOriginPoint(CENTER_OFFSET, CENTER_OFFSET);

    scene = new QGraphicsScene();
    scene->setSceneRect(0, 0, SCENE_SIZE, SCENE_SIZE);
    scene->setBackgroundBrush(QColor(60, 60, 103));
    scene->addItem(&pixmapItem);
    scene->addItem(&pixmapItemScaled);
    pixmapItemScaled.hide();

    setScene(scene);
}

void ImageViewerV2::initializeTimers()
{
    animationTimer = new QTimer(this);
    animationTimer->setSingleShot(true);

    scaleTimer = new QTimer(this);
    scaleTimer->setSingleShot(true);
    scaleTimer->setInterval(80);

    scrollAnimationX = new QPropertyAnimation(this, "scrollX", this);
    scrollAnimationX->setEasingCurve(QEasingCurve::OutSine);
    scrollAnimationX->setDuration(ANIMATION_SPEED);

    scrollAnimationY = new QPropertyAnimation(this, "scrollY", this);
    scrollAnimationY->setEasingCurve(QEasingCurve::OutSine);
    scrollAnimationY->setDuration(ANIMATION_SPEED);
}

void ImageViewerV2::initializeScrollBars()
{
    horizontalScroll = horizontalScrollBar();
    verticalScroll = verticalScrollBar();
}

void ImageViewerV2::setupConnections()
{
    connect(scrollAnimationX, &QPropertyAnimation::finished, this, &ImageViewerV2::onScrollTimelineFinished);
    connect(scrollAnimationY, &QPropertyAnimation::finished, this, &ImageViewerV2::onScrollTimelineFinished);
    connect(animationTimer, &QTimer::timeout, this, &ImageViewerV2::onAnimationTimer, Qt::UniqueConnection);
    connect(scaleTimer, &QTimer::timeout, this, &ImageViewerV2::requestScaling);
    connect(settings, &Settings::settingsChanged, this, &ImageViewerV2::readSettings);
}

bool ImageViewerV2::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() == QEvent::DevicePixelRatioChange) {
        onDPRChanged();
    }
    return QGraphicsView::eventFilter(obj, ev);
}

void ImageViewerV2::onDPRChanged()
{
    float newDpr = static_cast<float>(devicePixelRatioF());
    if (dpr == newDpr)
        return;

    dpr = newDpr;
    zoomThreshold = static_cast<int>(dpr * ZOOM_THRESHOLD_FACTOR);

    if (pixmap) {
        // 旧 dpr 下的一切都必须失效：缩放帧缓存、在途结果、请求去重锁存
        clearScaledFrameCache();
        mScaledFrameSize = QSize();
        invalidatePendingScaling();
        // 显式退回全尺寸图，不依赖 applyFitMode()/doZoom() 的副作用
        swapToOriginalPixmap();
        pixmap->setDevicePixelRatio(dpr);
        pixmapItem.setPixmap(*pixmap);
        pixmapItem.show();
        pixmapItem.update();
        updateMinScale();
        mSilentScaleChange = true;
        applyFitMode();
        mSilentScaleChange = false;
        requestScaling();
        update();
    }
}

int ImageViewerV2::scrollX() const
{
    return horizontalScroll ? horizontalScroll->value() : 0;
}

void ImageViewerV2::setScrollX(int x)
{
    scrollToX(x);
}

int ImageViewerV2::scrollY() const
{
    return verticalScroll ? verticalScroll->value() : 0;
}

void ImageViewerV2::setScrollY(int y)
{
    scrollToY(y);
}

// ----------------------------------------------------------------------------
// Internal non-virtual helpers
// ----------------------------------------------------------------------------
void ImageViewerV2::initSettings()
{
    transparencyGrid = settings->transparencyGrid();
    expandImage = settings->expandImage();
    expandLimit = static_cast<float>(settings->expandLimit());

    if (expandLimit < 1.0f)
        expandLimit = maxScale;

    keepFitMode = settings->keepFitMode();
    imageFitModeDefault = settings->imageFitMode();
    zoomStep = settings->zoomStep();
    focusIn1to1 = settings->focusPointIn1to1Mode();
    trackpadDetection = settings->trackpadDetection();

    useFixedZoomLevels = settings->useFixedZoomLevels();
    if (useFixedZoomLevels) {
        zoomLevels.clear();
        zoomLevels.reserve(16);

        const auto levelsStr = settings->zoomLevels().split(',');
        std::ranges::transform(levelsStr, std::back_inserter(zoomLevels), [](const QString& s) { return s.toFloat(); });

        std::sort(zoomLevels.begin(), zoomLevels.end());
    }

    onFullscreenModeChanged(mIsFullscreen);
    updateMinScale();
    setFitModeImpl(imageFitModeDefault);
}

void ImageViewerV2::setFitModeImpl(ImageFitMode mode)
{
    // 明确的模式切换：取消去抖中的请求，切完立即算一次
    scaleTimer->stop();

    stopPosAnimation();
    imageFitMode = mode;
    applyFitMode();
    requestScaling();
}

bool ImageViewerV2::imageFitsInternal() const
{
    if (!pixmap)
        return true;

    return (pixmap->width() <= static_cast<int>(static_cast<float>(viewport()->width()) * dpr) &&
            pixmap->height() <= static_cast<int>(static_cast<float>(viewport()->height()) * dpr));
}

float ImageViewerV2::currentScaleInternal() const
{
    return static_cast<float>(pixmapItem.scale());
}

// ----------------------------------------------------------------------------
// Virtual implementations (calling internal non-virtual functions)
// ----------------------------------------------------------------------------
void ImageViewerV2::readSettings()
{
    initSettings();
}

void ImageViewerV2::setFitMode(ImageFitMode mode)
{
    setFitModeImpl(mode);
}

bool ImageViewerV2::imageFits() const
{
    return imageFitsInternal();
}

float ImageViewerV2::currentScale() const
{
    return currentScaleInternal();
}

void ImageViewerV2::onFullscreenModeChanged(bool mode)
{
    mIsFullscreen = mode;
    QColor bgColor = mode ? settings->colorScheme().background_fullscreen
                          : settings->colorScheme().background;
    bgColor.setAlphaF(mode ? 1.0f : static_cast<float>(settings->backgroundOpacity()));
    scene->setBackgroundBrush(bgColor);
}

// ============================================================================
// Animation Control
// ============================================================================
void ImageViewerV2::startAnimation()
{
    // 显式起播（含用户按播放键）：不再等待首帧就绪，清掉延迟标志
    mStartAnimationOnFrameReady = false;
    // frameCount() == 0 表示帧数未知（imageCount() 扫描失败的可读动画），
    // 只有确知"仅一帧"时才不该起播；旧条件 > 1 会把未知帧数的动画永久判死
    if (movie && movie->frameCount() != 1) {
        stopAnimation();
        // ⭐ 暂停/切走后预解码会停手，恢复播放时继续在后台顺序解码
        movie->startDecoding();
        emit animationPaused(false);
        animationTimer->start(movie->nextFrameDelay());
    }
}

void ImageViewerV2::resumeAnimation()
{
    // 幻灯片启动时只恢复“已经暂停”的动画：
    // - 已在播放时不要重启计时器，否则会重置帧延迟并与进行中的预解码抢请求；
    // - 首帧仍在后台解码（mStartAnimationOnFrameReady）时由 showAnimationFrame()
    //   的就绪回调启动，提前开始会让计时器按旧播放头抢跑；
    // - 已知单帧/无效动画由 Core 的固定停留计时器处理，不在这里启动。
    if (!movie || mStartAnimationOnFrameReady || animationTimer->isActive())
        return;
    if (movie->frameCount() == 1)
        return;
    startAnimation();
}

void ImageViewerV2::stopAnimation()
{
    if (movie) {
        emit animationPaused(true);
        animationTimer->stop();
        // ⭐ 暂停只挂起解码任务：保留 reader 与已解码窗口，恢复时无需重解码前缀。
        // 真正切走/关闭时由 reset() 调 stopDecoding() 让任务退出、让出线程池
        movie->pauseDecoding();
    }
}

void ImageViewerV2::pauseResume()
{
    if (movie) {
        if (animationTimer->isActive())
            stopAnimation();
        else
            startAnimation();
    }
}

void ImageViewerV2::onAnimationTimer()
{
    if (!movie)
        return;

    const int count = movie->frameCount();
    int next = 0;

    if (count > 0) {
        // 帧数可能刚从"未知"（0）或虚高的扫描值被 EOF 修正：只剩一帧就没有可推进
        // 的目标（循环播放会原地重绘同一帧），停表即可。
        // 非循环（幻灯片）必须补发 playbackFinished，与下面的收尾分支一致：这时
        // Core 的固定停留计时器并没有挂（加载时帧数未知或 >1），不补发就再没有
        // 事件能推进幻灯片了。
        if (count == 1) {
            stopAnimation();
            if (!loopPlayback)
                emit playbackFinished();
            return;
        }
        const int last = count - 1;
        next = (movie->currentFrameNumber() >= last) ? 0 : movie->currentFrameNumber() + 1;

        if (next == 0 && !loopPlayback) {
            // 播放到末尾：交给 stopAnimation() 统一收尾（停表 + animationPaused(true)
            // + 让预解码停手），与下面几个失败分支保持一致
            stopAnimation();
            emit playbackFinished();
            return;
        }
    } else {
        // imageCount() 未知（0/-1）：不能计算 frameCount() - 1，顺序前进即可。
        // EOF 时后台会回填 correctedFrameCount，下一轮再按真实帧数判断循环边界。
        next = movie->currentFrameNumber() + 1;
    }

    if (!movie->jumpToFrame(next)) {
        // ⭐ 该帧仍在后台预解码：不阻塞 GUI、也不终止播放，稍后重试；
        // 预解码已停（缓存到末尾）说明这帧真的取不到，按原逻辑停止
        if (movie->isDecoding()) {
            animationTimer->start(FRAME_PENDING_RETRY_MS);
            return;
        }
        // 帧数在本次 jump 期间被 EOF 修正（截断/损坏动画）：重新走正常边界判断
        // （含循环/结束），否则会在末尾停住而不是回到首帧。
        const int refreshedCount = movie->frameCount();
        if (refreshedCount > 0 && refreshedCount != count) {
            animationTimer->start(0);
            return;
        }
        stopAnimation();
        if (!loopPlayback)
            emit playbackFinished();
        return;
    }

    // 缩小播放时直接走已缩放帧流水线，避免每帧全尺寸上传+绘制缩放
    if (pixmapItem.scale() < 1.0f) {
        // scaledSizeR() 已是设备像素（= 屏上实际占用的像素），再乘 dpr 会多出 dpr² 倍像素
        const QSize target = scaledSizeR();
        if (target.isValid() && !target.isEmpty()) {
            // ⭐ 直接在 QImage 域缩放到目标尺寸，只对缩小后的产物做 QPixmap 转换：
            // 全尺寸帧不再每帧转换/上传（4K 动画每帧可省数十 MB 像素拷贝）
            const auto frameImage = movie->getImage();
            if (!frameImage) {
                // 不留下已停住的 single-shot 定时器：能继续解码就重试，否则明确停止。
                // 非循环（幻灯片）时补发 playbackFinished，避免整场幻灯片永久停住。
                if (movie->isDecoding()) {
                    animationTimer->start(FRAME_PENDING_RETRY_MS);
                    return;
                }
                stopAnimation();
                if (!loopPlayback)
                    emit playbackFinished();
                return;
            }
            // ⭐ 循环播放时同一帧/同一目标尺寸只缩放一次
            setScaledPixmap(scaledFrameFor(movie->currentFrameNumber(), *frameImage, target), mViewerToken);
            emit frameChanged(movie->currentFrameNumber());
            animationTimer->start(movie->nextFrameDelay());
            return;
        }
    }

    // ImageAnimated::currentPixmap() 返回当前帧缓存（同一帧 + 同一 dpr 只转换一次）；
    // 缓存已带好当前 dpr，updatePixmap 设置同值时提前返回，不再整帧深拷贝
    QPixmap frame = movie->currentPixmap(static_cast<qreal>(dpr));
    if (frame.isNull()) {
        // 与上面的空 QImage 分支一致：要么安排重试，要么明确结束播放
        if (movie->isDecoding()) {
            animationTimer->start(FRAME_PENDING_RETRY_MS);
            return;
        }
        stopAnimation();
        if (!loopPlayback)
            emit playbackFinished();
        return;
    }
    updatePixmap(std::move(frame));

    emit frameChanged(movie->currentFrameNumber());
    animationTimer->start(movie->nextFrameDelay());
}

// 缩放帧缓存：按"当前目标尺寸"为键，尺寸变化即整体失效。
// 目标尺寸通常只在窗口缩放/变焦后变化，因此循环播放几乎不再重复缩放
QPixmap ImageViewerV2::scaledFrameFor(int frameNumber, const QImage &frame, const QSize &target)
{
    if (mScaledFrameSize != target) {
        clearScaledFrameCache();
        mScaledFrameSize = target;
    }

    auto it = mScaledFrameCache.find(frameNumber);
    if (it != mScaledFrameCache.end()) {
        // 命中即移到 LRU 尾部（MRU），保证淘汰顺序与真实使用顺序一致
        mScaledFrameOrder.removeOne(frameNumber);
        mScaledFrameOrder.append(frameNumber);
        return it.value();
    }

    // 缓存按约 32MB 估算上限：目标尺寸越大能缓存的帧越少（4K 全屏只缓 1 帧，
    // 此时没有多帧缓存收益），避免长动画在缩小播放时额外吃掉与全尺寸帧缓存同量级的内存
    // 乘法与移位在常量表达式内等价折算，但不触发 signed-bitwise 诊断
    constexpr qsizetype kCacheBudgetBytes = static_cast<qsizetype>(32) * 1024 * 1024;
    const qsizetype frameBytes = qMax<qsizetype>(1, static_cast<qsizetype>(target.width()) * target.height() * 4);
    const qsizetype maxFrames = qMax<qsizetype>(1, kCacheBudgetBytes / frameBytes);

    // 有界 LRU：达到上限后只淘汰最旧的一条，不再整表清空
    while (mScaledFrameCache.size() >= maxFrames) {
        if (mScaledFrameOrder.isEmpty()) {
            // 顺序表意外失配时整体失效兜底，避免缓存退化成无界
            clearScaledFrameCache();
            break;
        }
        mScaledFrameCache.remove(mScaledFrameOrder.takeFirst());
    }

    QPixmap scaled = QPixmap::fromImage(frame.scaled(target, Qt::KeepAspectRatio,
                                                      Qt::SmoothTransformation));
    // 缓存里就带好 dpr：setScaledPixmap() 再设置同值会提前返回，
    // 避免隐式共享的 pixmap 因修改 dpr 而每帧深拷贝一份像素
    scaled.setDevicePixelRatio(dpr);
    mScaledFrameCache.insert(frameNumber, scaled);
    mScaledFrameOrder.append(frameNumber);
    return scaled;
}

void ImageViewerV2::clearScaledFrameCache()
{
    mScaledFrameCache.clear();
    mScaledFrameOrder.clear();
}

void ImageViewerV2::nextFrame()
{
    if (!movie)
        return;

    int next = (movie->currentFrameNumber() == movie->frameCount() - 1)
               ? 0 : movie->currentFrameNumber() + 1;
    showAnimationFrame(next);
}

void ImageViewerV2::prevFrame()
{
    if (!movie)
        return;

    int prev = (movie->currentFrameNumber() == 0)
               ? movie->frameCount() - 1 : movie->currentFrameNumber() - 1;
    showAnimationFrame(prev);
}

bool ImageViewerV2::showAnimationFrame(int frame)
{
    if (!movie || frame < 0)
        return false;
    // frameCount() 为 0 表示帧数未知（可读动画），此时不能把第 0 帧判为越界
    const int count = movie->frameCount();
    if (count > 0 && frame >= count)
        return false;

    if (movie->currentFrameNumber() == frame) {
        if (mStartAnimationOnFrameReady) {
            mStartAnimationOnFrameReady = false;
            startAnimation();
        }
        return true;
    }

    if (!movie->jumpToFrame(frame)) {
        // ⭐ 逐帧浏览/暂停期间预解码可能已停手：先唤醒后台解码。
        // 已到末尾时 startDecoding() 不会启动新任务，此时缺帧就是真不可用。
        movie->startDecoding();
        // ⭐ 该帧还在后台解码：稍后再试一次（一次性定时器，不阻塞 GUI）；
        // 捕获动画实例：重试前若已切到别的动画，帧号相同也不能误用
        if (movie->isDecoding()) {
            const std::shared_ptr<ImageAnimated> animation = movie;
            QTimer::singleShot(FRAME_PENDING_RETRY_MS, this, [this, frame, animation]() {
                if (movie == animation && movie->currentFrameNumber() != frame)
                    showAnimationFrame(frame);
            });
        }
        return false;
    }

    emit frameChanged(movie->currentFrameNumber());
    // 缓存已按当前 dpr 打好标记；move 传递后视图侧设置同值不再拷贝
    updatePixmap(movie->currentPixmap(static_cast<qreal>(dpr)));
    // 切回动画时首帧已就绪：现在才开始播放计时，避免计时器从旧播放头抢跑
    if (mStartAnimationOnFrameReady) {
        mStartAnimationOnFrameReady = false;
        startAnimation();
    }
    return true;
}

// ============================================================================
// Display Operations
// ============================================================================
void ImageViewerV2::updatePixmap(const QPixmap& newPixmap)
{
    if (!pixmap)
        pixmap = std::make_unique<QPixmap>();

    *pixmap = newPixmap;
    pixmap->setDevicePixelRatio(dpr);

    pixmapItemScaled.hide();
    pixmapItem.setPixmap(*pixmap);
    if (!pixmapItem.isVisible())
        pixmapItem.show();
}

void ImageViewerV2::updatePixmap(QPixmap&& newPixmap)
{
    if (!pixmap)
        pixmap = std::make_unique<QPixmap>();

    *pixmap = std::move(newPixmap);
    pixmap->setDevicePixelRatio(dpr);

    pixmapItemScaled.hide();
    pixmapItem.setPixmap(*pixmap);
    if (!pixmapItem.isVisible())
        pixmapItem.show();
}

void ImageViewerV2::showAnimation(const std::shared_ptr<ImageAnimated>& animation)
{
    if (!animation || !animation->isValid())
        return;

    reset();
    movie = animation;
    // ⭐ 后续帧由 startAnimation() 启动后台预解码：GUI 线程不再同步 read()，
    // 帧未就绪时由 onAnimationTimer/showAnimationFrame 稍后重试
    movie->jumpToFrame(0);

    updatePixmap(movie->currentPixmap(static_cast<qreal>(dpr)));

    emit durationChanged(movie->frameCount());
    emit frameChanged(0);

    updateMinScale();

    applyInitialFit();

    if (movie->currentFrameNumber() == 0) {
        startAnimation();
    } else {
        // 环形缓冲可能已回收首帧（例如切走再切回动画）：先等后台解出首帧，
        // 由 showAnimationFrame 的重试在首帧就绪后启动播放。
        // 不能先启动计时器：它会按切走前的播放头继续请求后续帧，与首帧请求互相覆盖
        mStartAnimationOnFrameReady = true;
        showAnimationFrame(0);
    }
}

void ImageViewerV2::showImage(const QPixmap& newPixmap)
{
    showImageImpl(QPixmap(newPixmap));
}

void ImageViewerV2::showImage(QPixmap&& newPixmap)
{
    showImageImpl(std::move(newPixmap));
}

void ImageViewerV2::showImageImpl(QPixmap pixmapParam)
{
    // ⭐ 复用 reset() 清空旧图（含 viewport 位置重置与刷新），避免双份维护分叉
    reset();

    if(pixmapParam.isNull())
        return;

    pixmapItemScaled.hide();
    pixmap = std::make_unique<QPixmap>(std::move(pixmapParam));
    pixmap->setDevicePixelRatio(dpr);
    pixmapItem.setPixmap(*pixmap);
    pixmapItem.show();

    updateMinScale();

    applyInitialFit();

    // ⭐ setPixmap 已触发 scene 更新，不再 viewport()->update() 二次刷新
    requestScaling();
}

void ImageViewerV2::reset()
{
    // 换图/关闭/切动画都走这里：旧视图状态下的在途缩放结果与去抖请求一律作废
    scaleTimer->stop();
    invalidatePendingScaling();
    stopPosAnimation();
    // 缩放帧缓存按"当前动画 + 目标尺寸"有效，动画一变即失效
    clearScaledFrameCache();
    mScaledFrameSize = QSize();
    pixmapItemScaled.setPixmap(QPixmap());
    pixmapScaled = QPixmap();
    pixmapItem.setPixmap(QPixmap());
    pixmapItem.setScale(1.0f);
    pixmapItem.setOffset(CENTER_OFFSET, CENTER_OFFSET);
    pixmap.reset();
    stopAnimation();
    // 暂停只是挂起任务（保留 reader）；切走必须真正退出任务，
    // 否则单线程池会被挂起的任务占住，下一个动画永远排不到
    if (movie)
        movie->stopDecoding();
    mStartAnimationOnFrameReady = false;
    movie = nullptr;
    centerOn(CENTER_OFFSET, CENTER_OFFSET);
    viewport()->update();
}

void ImageViewerV2::closeImage()
{
    reset();
}

void ImageViewerV2::setScaledPixmap(const QPixmap& newFrame, quint64 viewerToken)
{
    // 令牌不匹配：结果来自换图/重置/切动画/DPR 变化之前的视图状态
    if (viewerToken != mViewerToken)
        return;
    if (!movie && newFrame.size() != scaledSizeR())
        return;

    pixmapScaled = newFrame;
    // worker 已按请求里的 dpr 打好标记，这里同值早退、不触发隐式共享解除；
    // 保留这一行是为了兜住"结果 dpr 与当前视图不一致"的旧调用路径。
    pixmapScaled.setDevicePixelRatio(dpr);
    pixmapItemScaled.setPixmap(pixmapScaled);
    pixmapItem.hide();
    pixmapItemScaled.show();
}

void ImageViewerV2::setScaledPixmap(QPixmap&& newFrame, quint64 viewerToken)
{
    if (viewerToken != mViewerToken)
        return;
    if (!movie && newFrame.size() != scaledSizeR())
        return;

    pixmapScaled = std::move(newFrame);
    pixmapScaled.setDevicePixelRatio(dpr);
    pixmapItemScaled.setPixmap(pixmapScaled);
    pixmapItem.hide();
    pixmapItemScaled.show();
}

bool ImageViewerV2::isDisplaying() const
{
    return (pixmap != nullptr);
}

// ============================================================================
// Scroll Operations
// ============================================================================
void ImageViewerV2::scrollUp()
{
    scroll(0, -DEFAULT_SCROLL_DISTANCE, true);
}

void ImageViewerV2::scrollDown()
{
    scroll(0, DEFAULT_SCROLL_DISTANCE, true);
}

void ImageViewerV2::scrollLeft()
{
    scroll(-DEFAULT_SCROLL_DISTANCE, 0, true);
}

void ImageViewerV2::scrollRight()
{
    scroll(DEFAULT_SCROLL_DISTANCE, 0, true);
}

void ImageViewerV2::scroll(int dx, int dy, bool animated)
{
    if (animated)
        scrollSmooth(dx, dy);
    else
        scrollPrecise(dx, dy);
}

void ImageViewerV2::scrollSmooth(int dx, int dy)
{
    auto updateAnimation = [this](QPropertyAnimation* animation, QScrollBar* scrollbar, int delta) {
        if (!delta)
            return;

        const int current = scrollbar->value();
        int newEnd = current + delta;

        const bool reversed = ((newEnd < current && current < animation->endValue().toInt()) ||
                               (newEnd > current && current > animation->endValue().toInt()));

        if (animation->state() == QAbstractAnimation::Running && !reversed) {
            newEnd = animation->endValue().toInt() + delta;
        }

        animation->stop();
        animation->setStartValue(current);
        animation->setEndValue(newEnd);
        animation->setDuration(ANIMATION_SPEED);
        animation->start();
    };

    if (dx)
        updateAnimation(scrollAnimationX, horizontalScroll, dx);

    if (dy)
        updateAnimation(scrollAnimationY, verticalScroll, dy);

    saveViewportPos();
}


void ImageViewerV2::scrollPrecise(int dx, int dy)
{
    stopPosAnimation();
    horizontalScroll->setValue(horizontalScroll->value() + dx);
    verticalScroll->setValue(verticalScroll->value() + dy);
    adjustViewport();
    saveViewportPos();
}

void ImageViewerV2::scrollToX(int x)
{
    horizontalScroll->setValue(x);
    adjustViewport();
    viewport()->update();
}

void ImageViewerV2::scrollToY(int y)
{
    verticalScroll->setValue(y);
    adjustViewport();
    viewport()->update();
}

void ImageViewerV2::onScrollTimelineFinished()
{
    saveViewportPos();
}

void ImageViewerV2::stopPosAnimation()
{
    if (scrollAnimationX->state() == QAbstractAnimation::Running)
        scrollAnimationX->stop();
    if (scrollAnimationY->state() == QAbstractAnimation::Running)
        scrollAnimationY->stop();
}

// ============================================================================
// Settings
// ============================================================================
void ImageViewerV2::toggleTransparencyGrid()
{
    transparencyGrid = !transparencyGrid;
    scene->update();
}

void ImageViewerV2::setLoopPlayback(bool mode)
{
    if (movie && mode && loopPlayback != mode)
        startAnimation();
    loopPlayback = mode;
}

void ImageViewerV2::setExpandImage(bool mode)
{
    expandImage = mode;
    updateMinScale();
    mSilentScaleChange = true;
    applyFitMode();
    mSilentScaleChange = false;
    requestScaling();
}

void ImageViewerV2::show()
{
    setMouseTracking(false);
    QGraphicsView::show();
    setMouseTracking(true);
}

void ImageViewerV2::hide()
{
    setMouseTracking(false);
    QWidget::hide();
}

void ImageViewerV2::requestScaling()
{
    if (!pixmap)
        return;

    const qreal scale = pixmapItem.scale();
    // 本次调用已经给出最终结论：先取消去抖中的旧请求，避免它稍后再回调一次。
    // 后续真实变焦都会经 scheduleScaling() 重新计时，不会丢请求。
    scaleTimer->stop();

    // 1:1 直出、放大由 QGraphicsView 直绘、动画帧有自己的缩放帧流水线：
    // 这三者都不需要后台 CPU 缩放结果，顺手作废在途结果与去重锁存
    if (movie || scale >= DIRECT_DRAW_SCALE) {
        invalidatePendingScaling();
        return;
    }

    // scaledSizeR() 是设备像素：结果 pixmap 会带 dpr 显示，
    // 若再乘 dpr，换入时尺寸会放大 dpr 倍且像素数多出 dpr² 倍
    const QSize requested = scaledSizeR();
    if (lastRequestedScale > 0 && qFuzzyCompare(1.0 + scale, 1.0 + lastRequestedScale)) {
        // avoid repeated identical scale requests
        return;
    }
    lastRequestedScale = static_cast<float>(scale);
    // dpr 是 float 成员，显式转 qreal：避免严格构建里 -Wdouble-promotion 的隐式提升告警
    emit scalingRequested(requested, static_cast<qreal>(dpr), mViewerToken);
}

void ImageViewerV2::enableDrags()
{
    dragsEnabled = true;
}

void ImageViewerV2::disableDrags()
{
    dragsEnabled = false;
}

// ============================================================================
// Zoom Operations (with common helper)
// ============================================================================
void ImageViewerV2::zoomIn()
{
    adjustZoom(true, false);
}

void ImageViewerV2::zoomInCursor()
{
    adjustZoom(true, true);
}

void ImageViewerV2::zoomOut()
{
    adjustZoom(false, false);
}

void ImageViewerV2::zoomOutCursor()
{
    adjustZoom(false, true);
}

void ImageViewerV2::adjustZoom(bool zoomIn, bool atCursor)
{
    if (atCursor && underMouse())
        setZoomAnchor(mapFromGlobal(cursor().pos()));
    else
        setZoomAnchor(viewport()->rect().center());

    const float current = currentScaleInternal();

    float newScale = zoomIn
        ? current * (1.0f + zoomStep)
        : current * (1.0f - zoomStep);

    if (useFixedZoomLevels && !zoomLevels.isEmpty()) [[unlikely]] {
        if (zoomIn) {
            if (current < zoomLevels.first()) {
                newScale = qMin(current * (1.0f + zoomStep), zoomLevels.first());
            } else if (current >= zoomLevels.last()) {
                newScale = current * (1.0f + zoomStep);
            } else {
                auto it = std::upper_bound(zoomLevels.cbegin(), zoomLevels.cend(), current);
                if (it != zoomLevels.cend())
                    newScale = *it;
            }
        } else {
            if (current > zoomLevels.last()) {
                newScale = qMax(zoomLevels.last(), current * (1.0f - zoomStep));
            } else if (current <= zoomLevels.first()) {
                newScale = current * (1.0f - zoomStep);
            } else {
                auto it = std::lower_bound(zoomLevels.cbegin(), zoomLevels.cend(), current);
                if (it != zoomLevels.cbegin())
                    newScale = *std::prev(it);
            }
        }
    }

    zoomAnchored(newScale);

    imageFitMode = FIT_FREE;

    if (pixmapItem.scale() == fitWindowScale)
        imageFitMode = FIT_WINDOW;
    else if (pixmapItem.scale() == fitWindowStretchScale)
        imageFitMode = FIT_WINDOW_STRETCH;
}

void ImageViewerV2::setZoomAnchor(QPoint viewportPos)
{
    zoomAnchor = QPair<QPointF, QPoint>(
        pixmapItem.mapFromScene(mapToScene(viewportPos)),
        viewportPos
    );
}

void ImageViewerV2::zoomAnchored(float newScale)
{
    const float current = currentScaleInternal();

    if (current == newScale) {
        // 缩放量不变时仍做一次边缘钳制，等价原调用方的 adjustViewport
        centerOn(clampViewportCenter(mapToScene(viewport()->rect().center())));
        return;
    }

    const QPoint viewportCenter = viewport()->rect().center();
    const QPointF sceneCenter = mapToScene(viewportCenter);

    doZoom(newScale, false);

    const QPointF anchorScene =
        pixmapItem.mapToScene(zoomAnchor.first);

    const QPoint mapped =
        mapFromScene(anchorScene);

    const QPointF diff =
        zoomAnchor.second - mapped;

    // 锚点保持 + 边缘钳制合并为一次 centerOn（原实现由调用方再调 adjustViewport）
    centerOn(clampViewportCenter(sceneCenter - diff));

    // ⭐ 交互变焦（滚轮/触控板/拖动）连续触发：这里只重排去抖定时器，
    // 停顿 80ms 后才请求一次缩放。期间视图用 item 自身的缩放做近似显示，
    // 不必为每一个中间缩放比例白算一整张图
    scheduleScaling();
}

void ImageViewerV2::scheduleScaling()
{
    scaleTimer->start();
}

void ImageViewerV2::doZoom(float newScale, bool center)
{
    if (!pixmap)
        return;

    newScale = qBound(minScale, newScale, maxScale);

    const float current = currentScaleInternal();
    if (qFuzzyCompare(1.0f + newScale, 1.0f + current))
        return;

    // ✅ 关键：保持当前 scene 中心不变（仅在调用方需要时执行）。
    // 必须在 setScale 之前计算，否则取到的是缩放后的中心。
    QPointF sceneCenterBefore;
    if (center)
        sceneCenterBefore = mapToScene(viewport()->rect().center());

    pixmapItem.setScale(newScale);
    swapToOriginalPixmap();

    if (center)
        centerOn(sceneCenterBefore);

    emit scaleChanged(newScale, mSilentScaleChange);
}

void ImageViewerV2::swapToOriginalPixmap()
{
    if (!pixmap || !pixmapItemScaled.isVisible())
        return;

    pixmapItemScaled.hide();
    pixmapItemScaled.setPixmap(QPixmap());
    pixmapScaled = QPixmap();
    pixmapItem.show();
}

// ============================================================================
// Fit Mode Operations
// ============================================================================
void ImageViewerV2::updateFitWindowScale()
{
    float scaleFitX = static_cast<float>(viewport()->width()) * dpr / static_cast<float>(pixmap->width());
    float scaleFitY = static_cast<float>(viewport()->height()) * dpr / static_cast<float>(pixmap->height());

    fitWindowScale = qMin(scaleFitX, scaleFitY);

    if (expandImage && fitWindowScale > expandLimit)
        fitWindowScale = expandLimit;
}

void ImageViewerV2::updateFitWindowStretchScale()
{
    if (!pixmap)
        return;

    fitWindowStretchScale = static_cast<float>(viewport()->height()) * dpr / static_cast<float>(pixmap->height());

    if (expandImage && fitWindowStretchScale > expandLimit)
        fitWindowStretchScale = expandLimit;
}

void ImageViewerV2::updateMinScale()
{
    if (!pixmap)
        return;

    updateFitWindowScale();
    updateFitWindowStretchScale();

    if (settings->unlockMinZoom()) {
        if (!pixmap->isNull())
            minScale = static_cast<float>(qMax(10. / pixmap->width(), 10. / pixmap->height()));
        else
            minScale = 1.0f;
    } else {
        minScale = imageFitsInternal() ? 1.0f : fitWindowScale;
    }

    if (mViewLock != LOCK_NONE && lockedScale < minScale)
        minScale = lockedScale;
}

void ImageViewerV2::fitWidth()
{
    if (!pixmap)
        return;

    float scaleX = static_cast<float>(viewport()->width()) * dpr / static_cast<float>(pixmap->width());

    if (!expandImage && scaleX > 1.0f)
        scaleX = 1.0f;
    if (scaleX > expandLimit)
        scaleX = expandLimit;

    if (currentScaleInternal() != scaleX) {
        swapToOriginalPixmap();
        doZoom(scaleX);
    }

    // 合并原 centerIfNecessary + 顶部对齐 + snapToEdges 的三次 centerOn 为一次，
    // 每次 centerOn 都会更新滚动条并触发重绘；先算出目标中心再统一钳制，
    // 最终位置与原实现完全一致
    const QRectF img = pixmapItem.sceneBoundingRect();
    const int viewportW = viewport()->width();
    const int viewportH = viewport()->height();

    QPointF center = mapToScene(viewport()->rect().center());
    if (qRound(img.width()) <= viewportW)
        center.setX(img.center().x());
    // 高度判断与 viewport 同为逻辑像素（scene 尺寸）：
    // scaledSizeR() 是设备像素，直接比较在 HiDPI 下会把本可垂直容纳的图误判为超高
    if (qRound(img.height()) <= viewportH)
        center.setY(img.center().y());
    else
        center.setY(0);

    centerOn(snappedCenter(center));
}

void ImageViewerV2::fitWindow()
{
    if (!pixmap)
        return;

    if (imageFitsInternal() && !expandImage) {
        fitNormal();
    } else {
        if (currentScaleInternal() != fitWindowScale) {
            swapToOriginalPixmap();
            doZoom(fitWindowScale);
        }

        if (scrollBarWorkaround) {
            scrollBarWorkaround = false;
            QTimer::singleShot(0, this, &ImageViewerV2::centerOnPixmap);
        } else {
            centerOnPixmap();
        }
    }
}

void ImageViewerV2::fitNormal()
{
    fitFree(1.0f);
}

void ImageViewerV2::fitWindowStretch()
{
    if (!pixmap)
        return;

    updateFitWindowStretchScale();

    if (currentScaleInternal() != fitWindowStretchScale) {
        swapToOriginalPixmap();
        doZoom(fitWindowStretchScale);
    }

    if (scrollBarWorkaround) {
        scrollBarWorkaround = false;
        QTimer::singleShot(0, this, &ImageViewerV2::centerOnPixmap);
    } else {
        centerOnPixmap();
    }
}

void ImageViewerV2::fitFree(float scale)
{
    if (!pixmap)
        return;

    if (focusIn1to1 == FOCUS_TOP) {
        doZoom(scale);

        // 与原逻辑一致：居中 + 可选顶部对齐后统一钳制，合并为单次 centerOn
        const QRectF img = pixmapItem.sceneBoundingRect();
        const int viewportH = viewport()->height();

        QPointF center = mapToScene(viewport()->rect().center());
        if (qRound(img.width()) <= viewport()->width())
            center.setX(img.center().x());
        // 高度判断与 viewport 同为逻辑像素（scene 尺寸），见 fitWidth() 中的说明
        if (qRound(img.height()) <= viewportH)
            center.setY(img.center().y());
        else {
            center.setX(img.center().x());
            center.setY(0);
        }

        centerOn(snappedCenter(center));
    } else {
        if (focusIn1to1 == FOCUS_CENTER)
            setZoomAnchor(viewport()->rect().center());
        else
            setZoomAnchor(mapFromGlobal(cursor().pos()));

        zoomAnchored(scale);
    }
}

void ImageViewerV2::applyInitialFit()
{
    // 打开图片后的自动适配属于非交互缩放：期间 scaleChanged 标记 silent，
    // 缩放指示器据此只同步数值、不触发淡入淡出
    mSilentScaleChange = true;

    if (!keepFitMode || imageFitMode == FIT_FREE)
        imageFitMode = imageFitModeDefault;

    if (mViewLock == LOCK_NONE) {
        applyFitMode();
    } else {
        imageFitMode = FIT_FREE;
        fitFree(lockedScale);
        if (mViewLock == LOCK_ALL)
            applySavedViewportPos();
    }

    mSilentScaleChange = false;
}

void ImageViewerV2::applyFitMode()
{
    switch (imageFitMode) {
        case FIT_ORIGINAL:       fitNormal(); break;
        case FIT_WIDTH:          fitWidth(); break;
        case FIT_WINDOW:         fitWindow(); break;
        case FIT_WINDOW_STRETCH: fitWindowStretch(); break;
        default: break;
    }
}

void ImageViewerV2::setFitOriginal()
{
    setFitModeImpl(FIT_ORIGINAL);
}

void ImageViewerV2::setFitWidth()
{
    setFitModeImpl(FIT_WIDTH);
}

void ImageViewerV2::setFitWindow()
{
    setFitModeImpl(FIT_WINDOW);
}

void ImageViewerV2::setFitWindowStretch()
{
    setFitModeImpl(FIT_WINDOW_STRETCH);
}

void ImageViewerV2::centerOnPixmap()
{
    if (!pixmap)
        return;

    const QRectF img = pixmapItem.sceneBoundingRect();

    const qreal x = img.center().x();
    const qreal y = img.center().y();

    centerOn(x, y);
}

QPointF ImageViewerV2::snappedCenter(QPointF center) const
{
    const QRectF img = pixmapItem.sceneBoundingRect();
    const QRectF vport = mapToScene(viewport()->rect()).boundingRect();

    const qreal vW = vport.width();
    const qreal vH = vport.height();

    // 贴边：以传入的目标中心为基准计算最终 vport，钳制到图片边缘内
    if (img.width() > vW) {
        if (img.left() > center.x() - vW / 2.0)
            center.rx() += img.left() - (center.x() - vW / 2.0);
        else if (img.right() < center.x() + vW / 2.0)
            center.rx() += img.right() - (center.x() + vW / 2.0);
    }

    if (img.height() > vH) {
        if (img.top() > center.y() - vH / 2.0)
            center.ry() += img.top() - (center.y() - vH / 2.0);
        else if (img.bottom() < center.y() + vH / 2.0)
            center.ry() += img.bottom() - (center.y() + vH / 2.0);
    }

    return center;
}

QPointF ImageViewerV2::clampViewportCenter(QPointF center) const
{
    if (!pixmap)
        return center;

    const QRectF img = pixmapItem.sceneBoundingRect();
    const int viewportW = viewport()->width();
    const int viewportH = viewport()->height();

    // 居中优先（与 centerIfNecessary 判断条件一致）
    if (qRound(img.width()) <= viewportW)
        center.setX(img.center().x());
    if (qRound(img.height()) <= viewportH)
        center.setY(img.center().y());

    return snappedCenter(center);
}

void ImageViewerV2::adjustViewport()
{
    if (!pixmap)
        return;

    const QPointF centerTarget = mapToScene(viewport()->rect().center());
    centerOn(clampViewportCenter(centerTarget));
}

// ============================================================================
// View Lock Operations
// ============================================================================
void ImageViewerV2::toggleLockZoom()
{
    if (!isDisplaying())
        return;

    if (mViewLock != LOCK_ZOOM) {
        mViewLock = LOCK_ZOOM;
        lockZoom();
    } else {
        mViewLock = LOCK_NONE;
    }
}

bool ImageViewerV2::lockZoomEnabled() const
{
    return (mViewLock == LOCK_ZOOM);
}

void ImageViewerV2::lockZoom()
{
    lockedScale = static_cast<float>(pixmapItem.scale());
    imageFitMode = FIT_FREE;
    saveViewportPos();
}

void ImageViewerV2::toggleLockView()
{
    if (!isDisplaying())
        return;

    if (mViewLock != LOCK_ALL) {
        mViewLock = LOCK_ALL;
        lockZoom();
        saveViewportPos();
    } else {
        mViewLock = LOCK_NONE;
    }
}

bool ImageViewerV2::lockViewEnabled() const
{
    return (mViewLock == LOCK_ALL);
}

void ImageViewerV2::saveViewportPos()
{
    if (mViewLock != LOCK_ALL)
        return;

    const QRectF itemRect = pixmapItem.sceneBoundingRect();

    const QPointF sceneCenter =
        mapToScene(viewport()->rect().center());

    savedViewportPos.setX(qBound(
        0.0,
        (sceneCenter.x() - itemRect.left()) / itemRect.width(),
        1.0));

    savedViewportPos.setY(qBound(
        0.0,
        (sceneCenter.y() - itemRect.top()) / itemRect.height(),
        1.0));
}

void ImageViewerV2::applySavedViewportPos()
{
    const QRectF itemRect = pixmapItem.sceneBoundingRect();

    const QPointF newScenePos(
        itemRect.left() + itemRect.width() * savedViewportPos.x(),
        itemRect.top() + itemRect.height() * savedViewportPos.y()
    );

    centerOn(newScenePos);
    adjustViewport();
}

// ============================================================================
// Mouse Event Handlers
// ============================================================================
void ImageViewerV2::mousePressEvent(QMouseEvent* event)
{
    if (!pixmap) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    mouseMoveStartPos = event->pos();
    mousePressPos = mouseMoveStartPos;

    // 拖拽/平移期间缩放与视口均不变，手势开始时预计算一次，避免每次 move 重复计算
    scaledImageFitsCache = scaledImageFits();

    if (event->button() & Qt::RightButton) {
        setZoomAnchor(event->pos());
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void ImageViewerV2::mouseMoveEvent(QMouseEvent* event)
{
    QGraphicsView::mouseMoveEvent(event);

    if (!pixmap || mouseInteraction == MOUSE_DRAG || mouseInteraction == MOUSE_WHEEL_ZOOM)
        return;

    if (event->buttons() & Qt::LeftButton) {
        if (mouseInteraction == MOUSE_NONE) {
            if (scaledImageFitsCache) {
                if (dragsEnabled)
                    mouseInteraction = MOUSE_DRAG_BEGIN;
            } else {
                mouseInteraction = MOUSE_PAN;
                if (cursor().shape() != Qt::ClosedHandCursor)
                    setCursor(Qt::ClosedHandCursor);
            }
        }

        if (mouseInteraction == MOUSE_DRAG_BEGIN) {
            if ((abs(mousePressPos.x() - event->pos().x()) > dragThreshold) ||
                (abs(mousePressPos.y() - event->pos().y()) > dragThreshold)) {
                mouseInteraction = MOUSE_NONE;
                emit draggedOut();
            }
        }

        if (mouseInteraction == MOUSE_PAN) {
            mousePan(event);
        }
        return;
    }

    if (event->buttons() & Qt::RightButton) {
        if (mouseInteraction == MOUSE_ZOOM ||
            static_cast<float>(std::abs(mousePressPos.y() - event->pos().y())) >
                static_cast<float>(zoomThreshold) / dpr) {
            if (cursor().shape() != Qt::SizeVerCursor)
                setCursor(Qt::SizeVerCursor);

            mouseInteraction = MOUSE_ZOOM;

            mouseMoveZoom(event);
        }
        return;
    }

    event->ignore();
}

void ImageViewerV2::mouseReleaseEvent(QMouseEvent* event)
{
    unsetCursor();

    if (!pixmap || mouseInteraction == MOUSE_NONE) {
        QGraphicsView::mouseReleaseEvent(event);
        event->ignore();
    }

    mouseInteraction = MOUSE_NONE;
}

void ImageViewerV2::mousePan(QMouseEvent* event)
{
    if (scaledImageFitsCache)
        return;

    mouseMoveStartPos -= event->pos();
    scroll(mouseMoveStartPos.x(), mouseMoveStartPos.y(), false);
    mouseMoveStartPos = event->pos();
    saveViewportPos();
}

void ImageViewerV2::mouseMoveZoom(QMouseEvent* event)
{
    const float stepMultiplier = 0.003f;
    int moveDistance = mouseMoveStartPos.y() - event->pos().y();
    float newScale = currentScaleInternal() * (1.0f + stepMultiplier * static_cast<float>(moveDistance) * dpr);
    mouseMoveStartPos = event->pos();
    imageFitMode = FIT_FREE;

    zoomAnchored(newScale);

    if (pixmapItem.scale() == fitWindowScale)
        imageFitMode = FIT_WINDOW;
    else if (pixmapItem.scale() == fitWindowStretchScale)
        imageFitMode = FIT_WINDOW_STRETCH;
}

void ImageViewerV2::wheelEvent(QWheelEvent* event)
{
#ifdef __APPLE__
    if (event->phase() == Qt::ScrollBegin || event->phase() == Qt::ScrollEnd) {
        event->accept();
        return;
    }
#endif

    if (event->buttons() & Qt::RightButton) {
        handleWheelZoom(event);
        return;
    }

    if (event->modifiers() != Qt::NoModifier) {
        event->ignore();
        QGraphicsView::wheelEvent(event);
        return;
    }

    QPoint angleDelta = event->angleDelta();

    bool isWheel = true;
    if (trackpadDetection) {
        if (wayland)
            isWheel = (event->phase() == Qt::NoScrollPhase);
        else
            isWheel = angleDelta.y() &&
                      (abs(angleDelta.y()) >= 120 &&
                      !(angleDelta.y() % 60)) &&
                      lastTouchpadScroll.elapsed() > 250;
    }

    if (!isWheel) {
        handleTrackpadScroll(event);
    } else if (settings->imageScrolling() == SCROLL_BY_TRACKPAD_AND_WHEEL) {
        handleMouseWheelScroll(event);
    } else {
        event->ignore();
        QGraphicsView::wheelEvent(event);
    }

    saveViewportPos();
}

void ImageViewerV2::handleWheelZoom(QWheelEvent* event)
{
    event->accept();
    mouseInteraction = MOUSE_WHEEL_ZOOM;
    int delta = event->angleDelta().ry();
    if (delta > 0)
        zoomInCursor();
    else if (delta < 0)
        zoomOutCursor();
}

void ImageViewerV2::handleTrackpadScroll(QWheelEvent* event)
{
    lastTouchpadScroll.restart();
    event->accept();

    if (settings->imageScrolling() != SCROLL_NONE) {
        stopPosAnimation();
        const QPoint pixelDelta = event->pixelDelta();
        const QPoint angleDelta = event->angleDelta();
        const int dx = qAbs(angleDelta.x()) > qAbs(pixelDelta.x()) ? angleDelta.x() : pixelDelta.x();
        const int dy = qAbs(angleDelta.y()) > qAbs(pixelDelta.y()) ? angleDelta.y() : pixelDelta.y();

        const int newH = qRound(horizontalScroll->value() - dx * TRACKPAD_SCROLL_MULTIPLIER);
        const int newV = qRound(verticalScroll->value() - dy * TRACKPAD_SCROLL_MULTIPLIER);

        horizontalScroll->setValue(newH);
        verticalScroll->setValue(newV);

        adjustViewport();
    }
}

void ImageViewerV2::handleMouseWheelScroll(QWheelEvent* event)
{
    QRect imgRect = scaledRectR();
    int delta = event->angleDelta().y();

    if ((delta < 0 && imgRect.bottom() > height() + 2) ||
        (delta > 0 && imgRect.top() < -2)) {
        event->accept();
        scroll(0, qRound(-delta * WHEEL_SCROLL_MULTIPLIER * settings->mouseScrollingSpeed()), true);
    } else {
        event->ignore();
    }
}

void ImageViewerV2::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    mousePressPos = mapFromGlobal(cursor().pos());

    if (parentWidget()->isVisible()) {
        stopPosAnimation();
        updateMinScale();

        if (imageFitMode == FIT_FREE || imageFitMode == FIT_ORIGINAL) {
            adjustViewport();
        } else {
            mSilentScaleChange = true;
            applyFitMode();
            mSilentScaleChange = false;
        }

        update();

        scheduleScaling();

        saveViewportPos();
    }
}

void ImageViewerV2::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);

    // 延迟执行 FIT_ORIGINAL 计算，避免嵌套事件循环和潜在的性能/递归问题。
    // 这和原来的 processEvents 目的一致：在控件显示后再重新布局/缩放。
    if (imageFitMode == FIT_ORIGINAL) {
        QTimer::singleShot(0, this, [this]() {
            if (imageFitMode == FIT_ORIGINAL) {
                mSilentScaleChange = true;
                applyFitMode();
                mSilentScaleChange = false;
            }
        });
    }
}

void ImageViewerV2::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawBackground(painter, rect);

    if (!isDisplaying() || !transparencyGrid || !pixmap->hasAlphaChannel())
        return;

    // 直接按已知状态算术计算 item 的 scene 矩形，避免每次绘制都走 QGraphicsItem
    // 矩阵换算：pixmapItem 位置恒为原点、offset 与 transformOriginPoint 均为
    // CENTER_OFFSET、仅 scale 变化，故 sceneBoundingRect = (offset, scale * size / dpr)
    const QPointF off = pixmapItem.offset();
    const qreal s = pixmapItem.scale();
    const qreal pixDpr = pixmap->devicePixelRatio();
    const QRectF itemRect(off.x(), off.y(),
                          static_cast<qreal>(pixmap->width()) / pixDpr * s,
                          static_cast<qreal>(pixmap->height()) / pixDpr * s);

    painter->drawTiledPixmap(rect.intersected(itemRect), checkboard);
}

// ============================================================================
// Query Methods
// ============================================================================
ImageFitMode ImageViewerV2::fitMode() const
{
    return imageFitMode;
}

bool ImageViewerV2::scaledImageFits() const
{
    if (!pixmap)
        return true;

    // scaledSizeR() 与 imageFitsInternal() 同为设备像素口径：
    // viewport 是逻辑像素，需乘 dpr 换算，否则 HiDPI 下会误判"图放不下"
    const QSize sz = scaledSizeR();
    return (sz.width() <= static_cast<int>(static_cast<float>(viewport()->width()) * dpr) &&
            sz.height() <= static_cast<int>(static_cast<float>(viewport()->height()) * dpr));
}

QWidget* ImageViewerV2::widget()
{
    return this;
}

bool ImageViewerV2::hasAnimation() const
{
    return (movie != nullptr);
}

QSize ImageViewerV2::scaledSizeR() const
{
    if (!pixmap)
        return QSize(0, 0);

    float s = currentScaleInternal();
    return QSize(qRound(static_cast<float>(pixmap->width()) * s), qRound(static_cast<float>(pixmap->height()) * s));
}

QRect ImageViewerV2::scaledRectR() const
{
    const QRectF mapped = mapFromScene(pixmapItem.sceneBoundingRect()).boundingRect();
    return QRect(mapped.topLeft().toPoint(), mapped.bottomRight().toPoint());
}

QSize ImageViewerV2::sourceSize() const
{
    if (!pixmap)
        return QSize(0, 0);
    return pixmap->size();
}

QPointF ImageViewerV2::sceneRoundPos(QPointF scenePoint) const
{
    return mapToScene(mapFromScene(scenePoint));
}

QRectF ImageViewerV2::sceneRoundRect(QRectF sceneRect) const
{
    return QRectF(sceneRoundPos(sceneRect.topLeft()), sceneRect.size());
}