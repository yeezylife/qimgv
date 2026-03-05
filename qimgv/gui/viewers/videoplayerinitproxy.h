// performs lazy initialization

#pragma once

#include <memory>
#include <QVBoxLayout>
#include <QPainter>
#include <QLibrary>
#include <QLabel>
#include <QFileInfo>
#include <QDebug>
#include "videoplayer.h"
#include "settings.h"

class VideoPlayerInitProxy : public VideoPlayer {
    Q_OBJECT
    Q_DISABLE_COPY(VideoPlayerInitProxy)
public:
    VideoPlayerInitProxy(QWidget *parent = nullptr);
    ~VideoPlayerInitProxy();

    bool showVideo(QString file) override;
    void seek(int pos) override;
    void seekRelative(int pos) override;
    void pauseResume() override;
    void frameStep() override;
    void frameStepBack() override;
    void stop() override;
    void setPaused(bool mode) override;
    void setMuted(bool mode) override;
    bool muted() override;
    void volumeUp() override;
    void volumeDown() override;
    void setVolume(int vol) override;
    int volume() override;
    void setVideoUnscaled(bool mode) override;
    void setLoopPlayback(bool mode) override;
    
    std::shared_ptr<VideoPlayer> getPlayer();
    bool isInitialized();

    void installEventFilter(QObject *filterObj) override;
    void removeEventFilter(QObject *filterObj) override;

public slots:
    void show();
    void hide();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLibrary playerLib;
    std::shared_ptr<VideoPlayer> player;
    bool initPlayer();
    QVBoxLayout layout;
    QLabel *errorLabel = nullptr;
    QObject *eventFilterObj = nullptr;

    QString libFile;
    QStringList libDirs;

    // 内联辅助函数
    inline bool isPlayerInitialized() const { return player != nullptr; }
    inline void checkPlayerInitialized(const char* funcName) const {
        if (!player) qDebug() << "Player not initialized, skipping" << funcName;
    }

private slots:
    void onSettingsChanged();

signals:
    void playbackFinished();
};