#include "videoplayerinitproxy.h"

#ifdef _QIMGV_PLAYER_PLUGIN
    #define QIMGV_PLAYER_PLUGIN _QIMGV_PLAYER_PLUGIN
#else
    #define QIMGV_PLAYER_PLUGIN ""
#endif

// 私有辅助函数减少重复的空指针检查
inline void VideoPlayerInitProxy::checkPlayerInitialized(const char* funcName) const {
    if (!player) {
        qDebug() << "Player not initialized, skipping " << funcName;
    }
}

inline bool VideoPlayerInitProxy::isPlayerInitialized() const {
    return player != nullptr;
}

VideoPlayerInitProxy::VideoPlayerInitProxy(QWidget *parent)
    : VideoPlayer(parent),
      player(nullptr)
{
    setAccessibleName("VideoPlayerInitProxy");
    setMouseTracking(true);
    layout.setContentsMargins(0,0,0,0);
    setLayout(&layout);
    connect(settings, &Settings::settingsChanged, this, &VideoPlayerInitProxy::onSettingsChanged);
    libFile = QIMGV_PLAYER_PLUGIN;
#ifdef _WIN32
    libDirs << QApplication::applicationDirPath() + "/plugins";
#else
    QDir libPath(QApplication::applicationDirPath() + "/../lib/qimgv");
    libDirs << (libPath.makeAbsolute() ? libPath.path() : ".") << "/usr/lib/qimgv" << "/usr/lib64/qimgv";
#endif
}

VideoPlayerInitProxy::~VideoPlayerInitProxy() {
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
    if(player)
        return true;

    QFileInfo pluginFile;
    for(const auto& dir : libDirs) {
        pluginFile.setFile(dir + "/" + libFile);
        if(pluginFile.isFile() && pluginFile.isReadable()) {
            playerLib.setFileName(pluginFile.absoluteFilePath());
            break; // 找到后立即退出循环，避免不必要的遍历
        }
    }
    if(playerLib.fileName().isEmpty()) {
        qDebug() << "Could not find" << libFile << "in the following directories:" << libDirs;
        return false;
    }

// load lib
    typedef VideoPlayer* (*createPlayerWidgetFn)();
    createPlayerWidgetFn fn = (createPlayerWidgetFn) playerLib.resolve("CreatePlayerWidget");
    if(fn) {
        VideoPlayer* pl = fn();
        player.reset(pl);
    }
    if(!player) {
        qDebug() << "Could not load:" << playerLib.fileName() << ". Wrong plugin version?";
        return false;
    }

    player->setMuted(!settings->playVideoSounds());
    player->setVideoUnscaled(!settings->expandImage());
    player->setVolume(settings->volume());

    player->setParent(this);
    layout.addWidget(player.get());
    player->hide();
    setFocusProxy(player.get());
    connect(player.get(), SIGNAL(durationChanged(int)), this, SIGNAL(durationChanged(int)));
    connect(player.get(), SIGNAL(positionChanged(int)), this, SIGNAL(positionChanged(int)));
    connect(player.get(), SIGNAL(videoPaused(bool)),    this, SIGNAL(videoPaused(bool)));
    connect(player.get(), SIGNAL(playbackFinished()),   this, SIGNAL(playbackFinished()));

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
    if (!isPlayerInitialized()) return;
    player->seek(pos);
}

void VideoPlayerInitProxy::seekRelative(int pos) {
    if (!isPlayerInitialized()) return;
    player->seekRelative(pos);
}

void VideoPlayerInitProxy::pauseResume() {
    if (!isPlayerInitialized()) return;
    player->pauseResume();
}

void VideoPlayerInitProxy::frameStep() {
    if (!isPlayerInitialized()) return;
    player->frameStep();
}

void VideoPlayerInitProxy::frameStepBack() {
    if (!isPlayerInitialized()) return;
    player->frameStepBack();
}

void VideoPlayerInitProxy::stop() {
    if (!isPlayerInitialized()) return;
    player->stop();
}

void VideoPlayerInitProxy::setPaused(bool mode) {
    if (!isPlayerInitialized()) return;
    player->setPaused(mode);
}

void VideoPlayerInitProxy::setMuted(bool mode) {
    if (!isPlayerInitialized()) return;
    player->setMuted(mode);
}

bool VideoPlayerInitProxy::muted() {
    if (!isPlayerInitialized()) return true;
    return player->muted();
}

void VideoPlayerInitProxy::volumeUp() {
    if (!isPlayerInitialized()) return;
    player->volumeUp();
    settings->setVolume(player->volume());
}

void VideoPlayerInitProxy::volumeDown() {
    if (!isPlayerInitialized()) return;
    player->volumeDown();
    settings->setVolume(player->volume());
}

void VideoPlayerInitProxy::setVolume(int vol) {
    if (!isPlayerInitialized()) return;
    player->setVolume(vol);
}

int VideoPlayerInitProxy::volume() {
    if (!isPlayerInitialized()) return 0;
    return player->volume();
}

void VideoPlayerInitProxy::setVideoUnscaled(bool mode) {
    if (!isPlayerInitialized()) return;
    player->setVideoUnscaled(mode);
}

void VideoPlayerInitProxy::setLoopPlayback(bool mode) {
    if (!isPlayerInitialized()) return;
    player->setLoopPlayback(mode);
}

void VideoPlayerInitProxy::show() {
    if(initPlayer()) {
        layout.removeWidget(errorLabel);
        player->show();
    } else if(!errorLabel) {
        errorLabel = new QLabel(this);
        errorLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        errorLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        //errorLabel->setAlignment(Qt::AlignVCenter);
        
        // 使用QStringBuilder优化字符串拼接
        QString errString = QString("Could not load %1 from:").arg(libFile);
        for(const auto& path : libDirs) {
            errString += QString("\n%1/").arg(path);
        }
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
