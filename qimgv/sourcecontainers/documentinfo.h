#pragma once

#include <QString>
#include <QHash>
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
#include <array>
#include <memory>

#include "utils/stuff.h"
#include "settings.h"

enum DocumentType { NONE, STATIC, ANIMATED, VIDEO };

class DocumentInfo {
public:
    explicit DocumentInfo(const QString &path);

    ~DocumentInfo() = default;

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

    void loadExifTags() const;
    const QHash<QString, QString>& getExifTags() const;

    bool isValid() const { return mDocumentType != NONE; }

private:
    QFileInfo fileInfo;

    DocumentType mDocumentType = NONE;
    int mOrientation = 0;
    QString mFormat;
    QMimeType mMimeType;

    mutable bool exifLoaded = false;
    mutable QHash<QString, QString> exifTags;

    // ✅ 新增缓存
    mutable QByteArray mHeaderCache;
    mutable bool mHeaderLoaded = false;
    mutable std::unique_ptr<QImageReader> mReader;

    static const QHash<QString, QString>& getKeyMapping();

    void detectFormat();
    void loadExifOrientation();

    bool detectAPNG();
    bool detectAnimatedWebP();
    bool detectAnimatedJxl();
    bool detectAnimatedAvif();

    int transformationToExifOrientation(QImageIOHandler::Transformations transformation) const;
    QString formatMetadataValue(const QString &key, const QVariant &value) const;

    // ✅ 新增工具函数
    const QByteArray& headerData(qint64 size = 256) const;
    QImageReader* getReader() const;
};