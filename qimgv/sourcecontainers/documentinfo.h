#pragma once

#include <QString>
#include <QMap>
#include <QSize>
#include <QUrl>
#include <QMimeDatabase>
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <QImageReader>
#include <cmath>
#include <cstring>
#include "utils/stuff.h"
#include "settings.h"

enum DocumentType { NONE, STATIC, ANIMATED, VIDEO };

class DocumentInfo {
public:
    DocumentInfo(QString path);
    ~DocumentInfo();
    
    QString directoryPath() const;
    QString filePath() const;
    QString fileName() const;
    QString baseName() const;
    qint64 fileSize() const;
    DocumentType type() const;
    QMimeType mimeType() const;

    // file extension (guessed from mime-type)
    QString format() const;
    int exifOrientation() const;

    QDateTime lastModified() const;
    void refresh();
    void loadExifTags();
    QMap<QString, QString> getExifTags();

private:
    QFileInfo fileInfo;
    DocumentType mDocumentType;
    int mOrientation;
    QString mFormat;
    bool exifLoaded;

    // 使用普通成员变量而非智能指针
    QMap<QString, QString> exifTags;

    // 静态函数返回映射表（解决翻译问题）
    static const QMap<QString, QString>& getKeyMapping();

    // guesses file type from its contents and sets extension
    void detectFormat();
    void loadExifOrientation();
    bool detectAPNG();
    bool detectAnimatedWebP();
    bool detectAnimatedJxl();
    bool detectAnimatedAvif();
    
    QMimeType mMimeType;
    
    // QImageIOHandler::Transformations 转换为标准 EXIF Orientation (1-8)
    int transformationToExifOrientation(QImageIOHandler::Transformations transformation) const;
    
    // 格式化元数据值
    QString formatMetadataValue(const QString &key, const QVariant &value) const;
};
