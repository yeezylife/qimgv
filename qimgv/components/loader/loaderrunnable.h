#pragma once

#include <QObject>
#include <QRunnable>
#include <QString>
#include <memory>
#include "utils/imagefactory.h"

class LoaderRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit LoaderRunnable(const QString& _path);
    void run() override;

private:
    QString path;

signals:
    void finished(std::shared_ptr<Image>, const QString &path);
};