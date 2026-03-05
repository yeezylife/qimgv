#include "videoplayerinitproxy.h"
#include <QApplication>
#include <QDir>

#ifdef _QIMGV_PLAYER_PLUGIN
    #define QIMGV_PLAYER_PLUGIN_STR _QIMGV_PLAYER_PLUGIN
#else
    #define QIMGV_PLAYER_PLUGIN_STR ""
#endif

VideoPlayerInitProxy::VideoPlayerInitProxy(QWidget *parent)
    : VideoPlayer(parent),
      player(nullptr),
      layout(this)
{
    setAccessibleName("VideoPlayerInitProxy");
    setMouseTracking(true);
    layout.setContentsMargins(0, 0, 0, 0);
    setLayout(&layout);

    connect(settings, &Settings::settingsChanged, this, &VideoPlayerInitProxy::onSettingsChanged);
    
    libFile = QIMGV_PLAYER_PLUGIN_STR;

#ifdef _WIN32
    libDirs << QApplication::applicationDirPath() + "/plugins";
#else
    QDir libPath(QApplication::applicationDirPath() + "/../lib/qimgv");
    libDirs << (libPath.exists() ? libPath.absolutePath() : ".") 
            << "/usr/lib/qimgv" 
            << "/usr/lib64/qimgv";
#endif
}

VideoPlayerInitProxy::~VideoPlayerInitProxy() {
    // shared_ptr 会自动处理 player 的析构
}

void VideoPlayerInitProxy::onSettingsChanged() {
    if (!player) return;
    player->setMuted(!settings->playVideoSounds());
    player->setVideoUnscaled(!settings->expandImage());
}

std::shared_ptr<VideoPlayer> VideoPlayerInitProxy::getPlayer() {
    return player;
}

bool VideoPlayerInitProxy::isInitialized() {
    return isPlayerInitialized();
}

bool VideoPlayerInitProxy::initPlayer() {
#ifndef USE_MPV
    return false;
#endif
    if (player) return true;

    QFileInfo pluginFile;
    bool found = false;
    for (const QString& dir : libDirs) {
        pluginFile.setFile(dir + "/" + libFile);
        if (pluginFile.isFile() && pluginFile.isReadable()) {
            playerLib.setFileName(pluginFile.absoluteFilePath());
            found = true;
            break;
        }
    }

    if (!found) {
        qDebug() << "Could not find" << libFile << "in:" << libDirs;
        return false;
    }

    typedef VideoPlayer* (*createPlayerWidgetFn)();
    createPlayerWidgetFn fn = (createPlayerWidgetFn)playerLib.resolve("CreatePlayerWidget");
    if (fn) {
        VideoPlayer* pl = fn();
        player.reset(pl);
    }

    if (!player) {
        qDebug() << "Could not load:" << playerLib.fileName() << ". Plugin error?";
        return false;
    }

    // 初始化配置
    player->setMuted(!settings->playVideoSounds());
    player->setVideoUnscaled(!settings->expandImage());
    player->setVolume(settings->volume());

    player->setParent(this);
    layout.addWidget(player.get());
    player->hide();
    setFocusProxy(player.get());

    // 信号转发
    connect(player.get(), SIGNAL(durationChanged(int)), this, SIGNAL(durationChanged(int)));
    connect(player.get(), SIGNAL(positionChanged(int)), this, SIGNAL(positionChanged(int)));
    connect(player.get(), SIGNAL(videoPaused(bool)),    this, SIGNAL(videoPaused(bool)));
    connect(player.get(), SIGNAL(playbackFinished()),   this, SIGNAL(playbackFinished()));

    if (eventFilterObj)
        player->installEventFilter(eventFilterObj);

    return true;
}

bool VideoPlayerInitProxy::showVideo(QString file) {
    if (!initPlayer()) return false;
    return player->showVideo(file);
}

void VideoPlayerInitProxy::seek(int pos) {
    if (isPlayerInitialized()) player->seek(pos);
}

void VideoPlayerInitProxy::seekRelative(int pos) {
    if (isPlayerInitialized()) player->seekRelative(pos);
}

void VideoPlayerInitProxy::pauseResume() {
    if (isPlayerInitialized()) player->pauseResume();
}

void VideoPlayerInitProxy::frameStep() {
    if (isPlayerInitialized()) player->frameStep();
}

void VideoPlayerInitProxy::frameStepBack() {
    if (isPlayerInitialized()) player->frameStepBack();
}

void VideoPlayerInitProxy::stop() {
    if (isPlayerInitialized()) player->stop();
}

void VideoPlayerInitProxy::setPaused(bool mode) {
    if (isPlayerInitialized()) player->setPaused(mode);
}

void VideoPlayerInitProxy::setMuted(bool mode) {
    if (isPlayerInitialized()) player->setMuted(mode);
}

bool VideoPlayerInitProxy::muted() {
    return isPlayerInitialized() ? player->muted() : true;
}

void VideoPlayerInitProxy::volumeUp() {
    if (isPlayerInitialized()) {
        player->volumeUp();
        settings->setVolume(player->volume());
    }
}

void VideoPlayerInitProxy::volumeDown() {
    if (isPlayerInitialized()) {
        player->volumeDown();
        settings->setVolume(player->volume());
    }
}

void VideoPlayerInitProxy::setVolume(int vol) {
    if (isPlayerInitialized()) player->setVolume(vol);
}

int VideoPlayerInitProxy::volume() {
    return isPlayerInitialized() ? player->volume() : 0;
}

void VideoPlayerInitProxy::setVideoUnscaled(bool mode) {
    if (isPlayerInitialized()) player->setVideoUnscaled(mode);
}

void VideoPlayerInitProxy::setLoopPlayback(bool mode) {
    if (isPlayerInitialized()) player->setLoopPlayback(mode);
}

void VideoPlayerInitProxy::show() {
    if (initPlayer()) {
        if (errorLabel) {
            layout.removeWidget(errorLabel);
            errorLabel->deleteLater();
            errorLabel = nullptr;
        }
        player->show();
    } else if (!errorLabel) {
        errorLabel = new QLabel(this);
        errorLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        errorLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        
        QString errString = QString("Could not load %1 from:").arg(libFile);
        for (const auto& path : libDirs) {
            errString += "\n" + path + "/";
        }
        errorLabel->setText(errString);
        layout.addWidget(errorLabel);
    }
    VideoPlayer::show();
}

void VideoPlayerInitProxy::hide() {
    if (player) player->hide();
    VideoPlayer::hide();
}

void VideoPlayerInitProxy::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)
}

void VideoPlayerInitProxy::installEventFilter(QObject *filterObj) {
    eventFilterObj = filterObj;
    if (player) player->installEventFilter(eventFilterObj);
}

void VideoPlayerInitProxy::removeEventFilter(QObject *filterObj) {
    eventFilterObj = nullptr;
    if (player) player->removeEventFilter(filterObj);
}