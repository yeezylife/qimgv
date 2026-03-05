#include "videoplayerinitproxy.h"

#ifdef _QIMGV_PLAYER_PLUGIN
#define QIMGV_PLAYER_PLUGIN _QIMGV_PLAYER_PLUGIN
#else
#define QIMGV_PLAYER_PLUGIN ""
#endif

VideoPlayerInitProxy::VideoPlayerInitProxy(QWidget *parent)
    : VideoPlayer(parent)
    , player(nullptr)
    , layout(this)
{
    setAccessibleName(QStringLiteral("VideoPlayerInitProxy"));
    setMouseTracking(true);
    layout.setContentsMargins(0, 0, 0, 0);
    setLayout(&layout);

    connect(settings, &Settings::settingsChanged, this, &VideoPlayerInitProxy::onSettingsChanged);

    libFile = QStringLiteral(QIMGV_PLAYER_PLUGIN);

#ifdef _WIN32
    libDirs << QApplication::applicationDirPath() + QStringLiteral("/plugins");
#else
    QDir libPath(QApplication::applicationDirPath() + QStringLiteral("/../lib/qimgv"));
    libDirs << (libPath.makeAbsolute() ? libPath.path() : QStringLiteral("."))
            << QStringLiteral("/usr/lib/qimgv")
            << QStringLiteral("/usr/lib64/qimgv");
#endif
}

VideoPlayerInitProxy::~VideoPlayerInitProxy() {
}

void VideoPlayerInitProxy::onSettingsChanged() {
    if (!player) return;
    player->setMuted(!settings->playVideoSounds());
    player->setVideoUnscaled(!settings->expandImage());
}

const std::shared_ptr<VideoPlayer>& VideoPlayerInitProxy::getPlayer() {
    return player;
}

bool VideoPlayerInitProxy::isInitialized() {
    return player != nullptr;
}

QStringList VideoPlayerInitProxy::buildSearchPaths() {
    QStringList paths;
    paths.reserve(libDirs.size());
    for (const auto& dir : libDirs) {
        paths << dir + QStringLiteral("/") + libFile;
    }
    return paths;
}

inline bool VideoPlayerInitProxy::initPlayer() {
#ifndef USE_MPV
    return false;
#endif

    if (player) return true;

    QFileInfo pluginFile;
    for (const auto& dir : libDirs) {
        pluginFile.setFile(dir + QStringLiteral("/") + libFile);
        if (pluginFile.isFile() && pluginFile.isReadable()) {
            playerLib.setFileName(pluginFile.absoluteFilePath());
            break;
        }
    }

    if (playerLib.fileName().isEmpty()) {
        qDebug() << QStringLiteral("Could not find plugin") << libFile
                 << QStringLiteral("in search paths:") << buildSearchPaths();
        return false;
    }

    typedef VideoPlayer* (*createPlayerWidgetFn)();
    createPlayerWidgetFn fn = reinterpret_cast<createPlayerWidgetFn>(
        playerLib.resolve("CreatePlayerWidget"));

    if (!fn) {
        qDebug() << QStringLiteral("Could not resolve CreatePlayerWidget function in:")
                 << playerLib.fileName();
        return false;
    }

    VideoPlayer* pl = fn();
    if (!pl) {
        qDebug() << QStringLiteral("CreatePlayerWidget returned null for:")
                 << playerLib.fileName();
        return false;
    }

    player.reset(pl);

    player->setMuted(!settings->playVideoSounds());
    player->setVideoUnscaled(!settings->expandImage());
    player->setVolume(settings->volume());

    player->setParent(this);
    layout.addWidget(player.get());
    player->hide();
    setFocusProxy(player.get());

    connect(player.get(), SIGNAL(durationChanged(int)), this, SIGNAL(durationChanged(int)));
    connect(player.get(), SIGNAL(positionChanged(int)), this, SIGNAL(positionChanged(int)));
    connect(player.get(), SIGNAL(videoPaused(bool)), this, SIGNAL(videoPaused(bool)));
    connect(player.get(), SIGNAL(playbackFinished()), this, SIGNAL(playbackFinished()));

    if (eventFilterObj) {
        player.get()->installEventFilter(eventFilterObj);
    }

    return true;
}

bool VideoPlayerInitProxy::showVideo(const QString &file) {
    if (!initPlayer()) return false;
    return player->showVideo(file);
}

void VideoPlayerInitProxy::seek(int pos) {
    if (!player) return;
    player->seek(pos);
}

void VideoPlayerInitProxy::seekRelative(int pos) {
    if (!player) return;
    player->seekRelative(pos);
}

void VideoPlayerInitProxy::pauseResume() {
    if (!player) return;
    player->pauseResume();
}

void VideoPlayerInitProxy::frameStep() {
    if (!player) return;
    player->frameStep();
}

void VideoPlayerInitProxy::frameStepBack() {
    if (!player) return;
    player->frameStepBack();
}

void VideoPlayerInitProxy::stop() {
    if (!player) return;
    player->stop();
}

void VideoPlayerInitProxy::setPaused(bool mode) {
    if (!player) return;
    player->setPaused(mode);
}

void VideoPlayerInitProxy::setMuted(bool mode) {
    if (!player) return;
    player->setMuted(mode);
}

bool VideoPlayerInitProxy::muted() {
    return !player || player->muted();
}

void VideoPlayerInitProxy::volumeUp() {
    if (!player) return;
    player->volumeUp();
    settings->setVolume(player->volume());
}

void VideoPlayerInitProxy::volumeDown() {
    if (!player) return;
    player->volumeDown();
    settings->setVolume(player->volume());
}

void VideoPlayerInitProxy::setVolume(int vol) {
    if (!player) return;
    player->setVolume(vol);
}

int VideoPlayerInitProxy::volume() {
    return player ? player->volume() : 0;
}

void VideoPlayerInitProxy::setVideoUnscaled(bool mode) {
    if (!player) return;
    player->setVideoUnscaled(mode);
}

void VideoPlayerInitProxy::setLoopPlayback(bool mode) {
    if (!player) return;
    player->setLoopPlayback(mode);
}

void VideoPlayerInitProxy::show() {
    if (initPlayer()) {
        if (errorLabel) {
            layout.removeWidget(errorLabel);
        }
        player->show();
    } else if (!errorLabel) {
        errorLabel = new QLabel(this);
        errorLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        errorLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        QString errString = QStringLiteral("Could not load %1 from:\n%2")
            .arg(libFile)
            .arg(buildSearchPaths().join(QStringLiteral("\n")));
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
    if (player) {
        player->installEventFilter(eventFilterObj);
    }
}

void VideoPlayerInitProxy::removeEventFilter(QObject *filterObj) {
    if (player) {
        player->removeEventFilter(filterObj);
    }
}
