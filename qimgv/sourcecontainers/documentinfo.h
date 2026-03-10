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
#include <QFile>
#include <QDataStream>
#include <cmath>
#include <cstring>

#include "utils/stuff.h"
#include "settings.h"

enum DocumentType { NONE, STATIC, ANIMATED, VIDEO };

class DocumentInfo {
public:
    explicit DocumentInfo(const QString &path);
    ~DocumentInfo();

    // 拷贝和移动操作（默认即可）
    DocumentInfo(const DocumentInfo &) = default;
    DocumentInfo& operator=(const DocumentInfo &) = default;
    DocumentInfo(DocumentInfo &&) noexcept = default;
    DocumentInfo& operator=(DocumentInfo &&) noexcept = default;

    QString directoryPath() const;
    QString filePath() const;
    QString fileName() const;
    QString baseName() const;
    qint64 fileSize() const;
    DocumentType type() const;
    QMimeType mimeType() const;
    QString format() const;
    int exifOrientation() const;
    QDateTime lastModified() const;

    void refresh();

    // 懒加载 EXIF 标签（const 正确版本）
    void loadExifTags() const;   // 注意：const 成员，可修改 mutable 成员
    const QMap<QString, QString>& getExifTags() const;   // 返回 const 引用

    bool isValid() const { return mDocumentType != NONE; }

private:
    QFileInfo fileInfo;

    DocumentType mDocumentType = NONE;
    int mOrientation = 0;
    QString mFormat;
    QMimeType mMimeType;

    // mutable 成员，允许在 const 函数中懒加载
    mutable bool exifLoaded = false;
    mutable QMap<QString, QString> exifTags;

    static const QMap<QString, QString>& getKeyMapping();

    void detectFormat();
    void loadExifOrientation();
    bool detectAPNG();
    bool detectAnimatedWebP();
    bool detectAnimatedJxl();
    bool detectAnimatedAvif();

    int transformationToExifOrientation(QImageIOHandler::Transformations transformation) const;
    QString formatMetadataValue(const QString &key, const QVariant &value) const;
};