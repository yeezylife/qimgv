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

#include "utils/stuff.h"

enum DocumentType { NONE, STATIC, ANIMATED, VIDEO };

class DocumentInfo {
public:
    // 任意线程可调用：jxl/video 开关已由调用方在 GUI 线程快照，视频表为进程级常量
    DocumentInfo(const QString &path, bool jxlAnimation, bool videoPlayback);

    ~DocumentInfo() = default;

    DocumentInfo(const DocumentInfo &) = delete;
    DocumentInfo& operator=(const DocumentInfo &) = delete;
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
    QDateTime lastModified() const;

    void refresh();

    void loadExifTags() const;
    const QHash<QString, QString>& getExifTags() const;
    // ⭐ 解码期已读的文本元数据与尺寸直接复用，避免面板显示时二次 QImageReader 打开
    void setCachedTextMetadata(QHash<QString, QString> metadata, QSize imageSize = {});

    bool isValid() const { return mDocumentType != NONE; }

private:
    QFileInfo fileInfo;

    DocumentType mDocumentType = NONE;
    QString mFormat;
    QMimeType mMimeType;

    mutable bool exifLoaded = false;
    mutable QHash<QString, QString> exifTags;
    // ⭐ 解码线程已缓存的原始文本元数据与尺寸（ImageStatic::loadGeneric 写入），
    // loadExifTags 优先使用，不再二次打开文件
    mutable QHash<QString, QString> mCachedRawText;
    mutable QSize mCachedImageSize;
    mutable bool mHasCachedRaw = false;

    // ✅ 仅保留安全缓存（不会占文件句柄）
    mutable QByteArray mHeaderCache;
    mutable bool mHeaderLoaded = false;

    static const QHash<QString, QString>& getKeyMapping();

    void detectFormat(bool jxlAnimation, bool videoPlayback);

    bool detectAPNG();
    bool detectAnimatedWebP();
    bool detectAnimatedAvif();

    QString formatMetadataValue(const QString &key, const QVariant &value) const;

    const QByteArray& headerData() const;
};