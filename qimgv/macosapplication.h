#pragma once

#include <QApplication>
#include <QFileOpenEvent>

class MacOSApplication : public QApplication {
    Q_OBJECT
public:
    MacOSApplication(int &argc, char *argv[]) : QApplication(argc, argv) {}
protected:
    bool event(QEvent *event) override {
        if (event->type() == QEvent::FileOpen) {
            emit fileOpened(static_cast<QFileOpenEvent *>(event)->file());
            return true;
        }
        return QApplication::event(event);
    }
signals:
    void fileOpened(QString filePath);
};
