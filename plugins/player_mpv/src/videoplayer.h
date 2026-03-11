#pragma once

#include <QWidget>

class VideoPlayer : public QWidget {
    Q_OBJECT
public:
    explicit VideoPlayer(QWidget *parent = nullptr);
    [[nodiscard]] virtual bool showVideo(const QString &file) = 0;
    virtual void seek(int pos) = 0;
    virtual void seekRelative(int pos) = 0;
    virtual void pauseResume() = 0;
    virtual void frameStep() = 0;
    virtual void frameStepBack() = 0;
    virtual void stop() = 0;
    virtual void setPaused(bool mode) = 0;
    virtual void setMuted(bool mode) = 0;
    [[nodiscard]] virtual bool muted() const = 0;
    virtual void volumeUp() = 0;
    virtual void volumeDown() = 0;
    virtual void setVolume(int vol) = 0;
    [[nodiscard]] virtual int volume() const = 0;
    virtual void setVideoUnscaled(bool mode) = 0;
    virtual void setLoopPlayback(bool mode) = 0;

signals:
    void durationChanged(int value);
    void positionChanged(int value);
    void videoPaused(bool);
    void playbackFinished();

public slots:
    virtual void show();
    virtual void hide();
};
