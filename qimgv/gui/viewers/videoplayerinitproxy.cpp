#include "videoplayerinitproxy.h"

#ifdef _QIMGV_PLAYER_PLUGIN
    #define QIMGV_PLAYER_PLUGIN _QIMGV_PLAYER_PLUGIN
#else
    #define QIMGV_PLAYER_PLUGIN ""
#endif

// 常量定义
static constexpr const char* PLUGIN_FILE_NAME = QIMGV_PLAYER_PLUGIN;
static constexpr const char* PLUGIN_SUBDIR = "plugins";
static constexpr const char* WINDOWS_PLUGIN_DIR = "plugins";
static constexpr const char* LINUX_PLUGIN_DIR = "../lib/qimgv";
static constexpr const char* LINUX_ALT_PLUGIN_DIR1 = "/usr/lib/qimgv";
static constexpr const char* LINUX_ALT_PLUGIN_DIR2 = "/usr/lib64/qimgv";

VideoPlayerInitProxy::VideoPlayerInitProxy(QWidget *parent)
    : VideoPlayer(parent),
      player(nullptr)
{
    setAccessibleName("VideoPlayerInitProxy");
    setMouseTracking(true);
    layout.setContentsMargins(0,0,0,0);
    setLayout(&layout);
    connect(settings, &Settings::settingsChanged, this, &VideoPlayerInitProxy::onSettingsChanged);
    libFile = PLUGIN_FILE_NAME;
    
#ifdef _WIN32
    libDirs << QApplication::applicationDirPath() + "/" + WINDOWS_PLUGIN_DIR;
#else
    QDir libPath(QApplication::applicationDirPath() + "/" + LINUX_PLUGIN_DIR);
    libDirs << (libPath.makeAbsolute() ? libPath.path() : ".") 
           << LINUX_ALT_PLUGIN_DIR1 << LINUX_ALT_PLUGIN_DIR2;
#endif
}

VideoPlayerInitProxy::~VideoPlayerInitProxy() {
    // 修复：解除父子关系，避免 double delete
    if (player) {
        player->setParent(nullptr);
        player.reset();
    }
}

void VideoPlayerInitProxy::onSettingsChanged() {
    if(!player)
        return;
    player->setMuted(!settings->playVideoSounds());
    player->setVideoUnscaled(!settings->expandImage());
}

std::shared_ptr<VideoPlayer> VideoPlayerInitProxy::getPlayer() {
    return player;
}

bool VideoPlayerInitProxy::isInitialized() {
    return (player != nullptr);
}

inline bool VideoPlayerInitProxy::initPlayer() {
#ifndef USE_MPV
    return false;
#endif
    // 如果已经初始化，直接返回
    if(player)
        return true;

    // 搜索插件文件
    QFileInfo pluginFile;
    for(const auto& dir : libDirs) {
        pluginFile.setFile(dir + "/" + libFile);
        if(pluginFile.isFile() && pluginFile.isReadable()) {
            playerLib.setFileName(pluginFile.absoluteFilePath());
            break;
        }
    }
    
    // 检查是否找到插件
    if(playerLib.fileName().isEmpty()) {
        QStringList searchPaths;
        for(const auto& dir : libDirs) {
            searchPaths << (dir + "/" + libFile);
        }
        qDebug() << "Could not find plugin" << libFile << "in search paths:" << searchPaths;
        return false;
    }

    // 加载插件库
    typedef VideoPlayer* (*createPlayerWidgetFn)();
    createPlayerWidgetFn fn = reinterpret_cast<createPlayerWidgetFn>(playerLib.resolve("CreatePlayerWidget"));
    if(!fn) {
        qDebug() << "Could not resolve CreatePlayerWidget function in:" << playerLib.fileName();
        return false;
    }
    
    // 创建播放器实例
    VideoPlayer* pl = fn();
    if(!pl) {
        qDebug() << "CreatePlayerWidget returned null for:" << playerLib.fileName();
        return false;
    }
    
    player.reset(pl);

    // 配置播放器
    player->setMuted(!settings->playVideoSounds());
    player->setVideoUnscaled(!settings->expandImage());
    player->setVolume(settings->volume());

    // 设置父子关系和布局
    player->setParent(this);
    layout.addWidget(player.get());
    player->hide();
    setFocusProxy(player.get());
    
    // 连接信号
    connect(player.get(), SIGNAL(durationChanged(int)), this, SIGNAL(durationChanged(int)));
    connect(player.get(), SIGNAL(positionChanged(int)), this, SIGNAL(positionChanged(int)));
    connect(player.get(), SIGNAL(videoPaused(bool)),    this, SIGNAL(videoPaused(bool)));
    connect(player.get(), SIGNAL(playbackFinished()),   this, SIGNAL(playbackFinished()));

    // 安装事件过滤器
    if(eventFilterObj)
        player.get()->installEventFilter(eventFilterObj);

    return true;
}

bool VideoPlayerInitProxy::showVideo(QString file) {
    if(!initPlayer())
        return false;
    return player->showVideo(file);
}

void VideoPlayerInitProxy::seek(int pos) {
    if(!player) return;
    player->seek(pos);
}

void VideoPlayerInitProxy::seekRelative(int pos) {
    if(!player) return;
    player->seekRelative(pos);
}

void VideoPlayerInitProxy::pauseResume() {
    if(!player) return;
    player->pauseResume();
}

void VideoPlayerInitProxy::frameStep() {
    if(!player) return;
    player->frameStep();
}

void VideoPlayerInitProxy::frameStepBack() {
    if(!player) return;
    player->frameStepBack();
}

void VideoPlayerInitProxy::stop() {
    if(!player) return;
    player->stop();
}

void VideoPlayerInitProxy::setPaused(bool mode) {
    if(!player) return;
    player->setPaused(mode);
}

void VideoPlayerInitProxy::setMuted(bool mode) {
    if(!player) return;
    player->setMuted(mode);
}

bool VideoPlayerInitProxy::muted() {
    return !player || player->muted();
}

void VideoPlayerInitProxy::volumeUp() {
    if(!player) return;
    player->volumeUp();
    settings->setVolume(player->volume());
}

void VideoPlayerInitProxy::volumeDown() {
    if(!player) return;
    player->volumeDown();
    settings->setVolume(player->volume());
}

void VideoPlayerInitProxy::setVolume(int vol) {
    if(!player) return;
    player->setVolume(vol);
}

int VideoPlayerInitProxy::volume() {
    return player ? player->volume() : 0;
}

void VideoPlayerInitProxy::setVideoUnscaled(bool mode) {
    if(!player) return;
    player->setVideoUnscaled(mode);
}

void VideoPlayerInitProxy::setLoopPlayback(bool mode) {
    if(!player) return;
    player->setLoopPlayback(mode);
}

void VideoPlayerInitProxy::show() {
    if(initPlayer()) {
        // 成功加载时清理错误标签
        if (errorLabel) {
            delete errorLabel;
            errorLabel = nullptr;
        }
        player->show();
    } else if(!errorLabel) {
        errorLabel = new QLabel(this);
        errorLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        errorLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        
        // 构建错误消息
        QStringList searchPaths;
        for(const auto& path : libDirs) {
            searchPaths << (path + "/");
        }
        QString errString = QString("Could not load %1 from:\n%2")
                                .arg(libFile)
                                .arg(searchPaths.join("\n"));
        
        errorLabel->setText(errString);
        layout.addWidget(errorLabel);
    }
    VideoPlayer::show();
}

void VideoPlayerInitProxy::hide() {
    if(player)
        player->hide();
    VideoPlayer::hide();
}

void VideoPlayerInitProxy::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
}

void VideoPlayerInitProxy::installEventFilter(QObject *filterObj) {
    eventFilterObj = filterObj;
    if(player)
        player->installEventFilter(eventFilterObj);
}

void VideoPlayerInitProxy::removeEventFilter(QObject *filterObj) {
    if(player)
        player->removeEventFilter(filterObj);
}