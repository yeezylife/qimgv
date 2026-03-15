#pragma once

#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QString>
#include "core.h"

class CmdOptionsRunner : public QObject {
    Q_OBJECT
public slots:
    // 修改为常引用以优化性能
    void generateThumbs(const QString &dirPath, int size);
    void showBuildOptions();
};