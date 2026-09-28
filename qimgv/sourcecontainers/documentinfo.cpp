#include "documentinfo.h"
#include "videoformats.h"
#include <QtEndian>

using namespace Qt::StringLiterals;

// ⭐ 进程内只查询一次，避免每个 PNG 都重建格式列表
static bool hasApngFormat() {
    static const bool hasApng = QImageReader::supportedImageFormats().contains("apng");
    return hasApng;
}

// ====================== header cache ======================

const QByteArray& DocumentInfo::headerData() const {
    if (!mHeaderLoaded) {
        QFile f(fileInfo.filePath());
        if (f.open(QFile::ReadOnly)) {
            mHeaderCache = f.read(128);
        }
        mHeaderLoaded = true;
    }
    return mHeaderCache;
}

// ====================== Key Mapping ======================

const QHash<QString, QString>& DocumentInfo::getKeyMapping() {
    static const QHash<QString, QString> mapping = {
        {u"Make"_s, QObject::tr("Make")},
        {u"Model"_s, QObject::tr("Model")},
        {u"DateTime"_s, QObject::tr("Date/Time")},
        {u"ExposureTime"_s, QObject::tr("ExposureTime")},
        {u"FNumber"_s, QObject::tr("F Number")},
        {u"ISOSpeedRatings"_s, QObject::tr("ISO Speed ratings")},
        {u"Flash"_s, QObject::tr("Flash")},
        {u"FocalLength"_s, QObject::tr("Focal Length")},
        {u"UserComment"_s, QObject::tr("UserComment")},
    };
    return mapping;
}

// ====================== ctor ======================

DocumentInfo::DocumentInfo(const QString &path, bool jxlAnimation, bool videoPlayback) {
    fileInfo.setFile(path);

    if(!fileInfo.isFile()) {
        qDebug() << "FileInfo: cannot open:" << path;
        return;
    }

    detectFormat(jxlAnimation, videoPlayback);
}

// ====================== getters ======================

QString DocumentInfo::directoryPath() const { return fileInfo.absolutePath(); }
QString DocumentInfo::filePath() const { return fileInfo.absoluteFilePath(); }
QString DocumentInfo::fileName() const { return fileInfo.fileName(); }
QString DocumentInfo::baseName() const { return fileInfo.baseName(); }
qint64 DocumentInfo::fileSize() const { return fileInfo.size(); }
DocumentType DocumentInfo::type() const { return mDocumentType; }
QMimeType DocumentInfo::mimeType() const { return mMimeType; }
QString DocumentInfo::format() const { return mFormat; }
QDateTime DocumentInfo::lastModified() const { return fileInfo.lastModified(); }

void DocumentInfo::refresh() { fileInfo.refresh(); }

// ====================== detect ======================

void DocumentInfo::detectFormat(bool jxlAnimation, bool videoPlayback) {

    if(mDocumentType != NONE)
        return;

    static QMimeDatabase mimeDb;

    // ⭐ 只读一次文件头并缓存；常见格式直接按魔数判定，省掉 MIME 探测的那次文件读取
    const QByteArray &header = headerData();

    if(header.size() >= 2) {
        const uchar *d = reinterpret_cast<const uchar*>(header.constData());

        if(header.size() >= 8
                && d[0] == 0x89 && d[1] == 'P' && d[2] == 'N' && d[3] == 'G'
                && d[4] == 0x0D && d[5] == 0x0A && d[6] == 0x1A && d[7] == 0x0A) {

            if(hasApngFormat() && detectAPNG()) {
                mFormat = "apng";
                mDocumentType = ANIMATED;
            } else {
                mFormat = "png";
                mDocumentType = STATIC;
            }
            return;
        }

        if(header.size() >= 3 && d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF) {
            mFormat = "jpg";
            mDocumentType = STATIC;
            return;
        }

        if(header.size() >= 6 && (std::memcmp(d, "GIF87a", 6) == 0 || std::memcmp(d, "GIF89a", 6) == 0)) {
            mFormat = "gif";
            mDocumentType = ANIMATED;
            return;
        }

        if(header.size() >= 12 && std::memcmp(d, "RIFF", 4) == 0 && std::memcmp(d + 8, "WEBP", 4) == 0) {
            mFormat = "webp";
            mDocumentType = detectAnimatedWebP() ? ANIMATED : STATIC;
            return;
        }

        if(header.size() >= 12 && std::memcmp(d + 4, "ftypavis", 8) == 0) {
            mFormat = "avif";
            mDocumentType = detectAnimatedAvif() ? ANIMATED : STATIC;
            return;
        }

        if(d[0] == 'B' && d[1] == 'M') {
            mFormat = "bmp";
            mDocumentType = STATIC;
            return;
        }
    }

    // 魔数无法判定（视频、未知格式等）时回落 MIME 探测，保留原有行为
    mMimeType = mimeDb.mimeTypeForFile(fileInfo, QMimeDatabase::MatchDefault);

    const QByteArray mimeName = mMimeType.name().toLatin1();
    const QByteArray suffix = fileInfo.suffix().toLower().toLatin1();

    if(mimeName == "image/jpeg") {

        mFormat = "jpg";
        mDocumentType = STATIC;

    } else if(mimeName == "image/png") {

        if(hasApngFormat() && detectAPNG()) {
            mFormat = "apng";
            mDocumentType = ANIMATED;
        } else {
            mFormat = "png";
            mDocumentType = STATIC;
        }

    } else if(mimeName == "image/gif") {

        mFormat = "gif";
        mDocumentType = ANIMATED;

    } else if(mimeName == "image/webp" || (mimeName == "audio/x-riff" && suffix == "webp")) {

        mFormat = "webp";
        mDocumentType = detectAnimatedWebP() ? ANIMATED : STATIC;

    } else if(mimeName == "image/jxl") {

        mFormat = "jxl";

        QImageReader reader(fileInfo.absoluteFilePath(), "jxl");
        mDocumentType = reader.supportsAnimation() ? ANIMATED : STATIC;

        if(mDocumentType == ANIMATED && !jxlAnimation) {
            mDocumentType = NONE;
            qDebug() << "animated jxl disabled";
        }

    } else if(mimeName == "image/avif") {

        mFormat = "avif";
        mDocumentType = detectAnimatedAvif() ? ANIMATED : STATIC;

    } else if(mimeName == "image/bmp") {

        mFormat = "bmp";
        mDocumentType = STATIC;

    } else {

        mFormat = suffix;

        if(QStringView(mFormat).compare(u"jfif", Qt::CaseInsensitive) == 0)
            mFormat = "jpg";

        if(videoPlayback) {

            // 视频表/后缀表均为进程级常量（videoformats.h），并发只读安全；
            // 单次 constFind 兼顾命中判定与取值，避免 contains+value 两次查找
            const auto &table = videoFormatTable();
            if(const auto it = table.constFind(mimeName); it != table.constEnd()) {
                mDocumentType = VIDEO;
                mFormat = it.value();
            } else if(videoFormatSuffixes().contains(suffix)) {
                mDocumentType = VIDEO;
            } else {
                mDocumentType = STATIC;
            }

        } else {
            mDocumentType = STATIC;
        }
    }

}

// ====================== detect impl ======================

bool DocumentInfo::detectAPNG() {
    const QByteArray& buf = headerData();

    // 最少需要：8 字节 PNG 签名 + 4 字节 chunk 长度 + 4 字节 chunk 类型
    if (buf.size() < 16)
        return false;

    // ⭐ 按 PNG chunk 结构定位 acTL（必须在首个 IDAT 之前），避免全串扫描
    qsizetype pos = 8;
    while (pos + 8 <= buf.size()) {
        const char *chunk = buf.constData() + pos;

        if (std::memcmp(chunk + 4, "acTL", 4) == 0)
            return true;

        if (std::memcmp(chunk + 4, "IDAT", 4) == 0)
            break;

        const quint32 len = qFromBigEndian<quint32>(chunk);
        const qsizetype next = pos + 8 + len + 4; // length + type + data + crc
        if (next <= pos)
            break;
        pos = next;
    }

    return false;
}

bool DocumentInfo::detectAnimatedWebP() {
    const QByteArray& buf = headerData();

    if (buf.size() < 21)
        return false;

    if (std::memcmp(buf.constData() + 12, "VP8X", 4) != 0)
        return false;

    return buf[20] & 0x02;
}

bool DocumentInfo::detectAnimatedAvif() {
    const QByteArray& buf = headerData();

    if (buf.size() < 12)
        return false;

    return std::memcmp(buf.constData() + 4, "ftypavis", 8) == 0;
}

// ====================== metadata ======================

QString DocumentInfo::formatMetadataValue(const QString &key,const QVariant &value) const {

    if(key == u"ExposureTime"_s) {

        bool ok;
        double t = value.toDouble(&ok);

        if(ok && t>0) {
            if(t < 1.0)
                return QString("1/%1 %2").arg(qRound(1.0/t)).arg(QObject::tr("sec"));

            return QString::number(t, 'f', 2) + u' ' + QObject::tr("sec");
        }
    }

    else if(key == u"FNumber"_s || key == u"ApertureValue"_s) {

        bool ok;
        double f = value.toDouble(&ok);

        if(ok && f>0)
            return QString("f/%1").arg(f,0,'g',3);
    }

    else if(key == u"FocalLength"_s) {

        bool ok;
        double fl = value.toDouble(&ok);

        if(ok && fl>0)
            return QString("%1 mm").arg(fl,0,'g',3);
    }

    return value.toString();
}

void DocumentInfo::setCachedTextMetadata(QHash<QString, QString> metadata) {
    // ⭐ 解码期写入，GUI 线程首次 getExifTags 前已完成（Image 构造后才进缓存/面板）
    mCachedRawText = std::move(metadata);
    mHasCachedRaw = true;
}

void DocumentInfo::loadExifTags() const {

    if(exifLoaded)
        return;

    exifLoaded = true;
    exifTags.clear();

    const auto &mapping = getKeyMapping();

    // ⭐ 优先复用解码期缓存，不再二次 QImageReader 打开+解析
    if(mHasCachedRaw) {
        for(auto it = mCachedRawText.constBegin(); it != mCachedRawText.constEnd(); ++it) {
            const QString &key = it.key();
            const QString &value = it.value();
            if(value.isEmpty())
                continue;
            QString displayKey = mapping.value(key, key);
            QString formattedValue = formatMetadataValue(key, value);
            if(key == u"UserComment"_s && formattedValue.startsWith(u"charset="_s)) {
                qsizetype spaceIndex = formattedValue.indexOf(u' ');
                if(spaceIndex > 0)
                    formattedValue = formattedValue.mid(spaceIndex + 1);
            }
            exifTags.try_emplace(displayKey, std::move(formattedValue));
        }
        if(!exifTags.isEmpty())
            return;
        // 缓存为空（如 PNG 无文本块）则回落按尺寸填充，避免再开文件
        // 尺寸由调用方经 Image::size() 获取，此处不再 reader.size() 二次解析
        return;
    }

    QImageReader reader(fileInfo.absoluteFilePath());

    if(!reader.canRead())
        return;

    const QStringList textKeys = reader.textKeys();

    for(const QString &key : textKeys) {

        const QString value = reader.text(key);
        if(value.isEmpty())
            continue;

        QString displayKey = mapping.value(key, key);
        QString formattedValue = formatMetadataValue(key, value);

        if(key == u"UserComment"_s && formattedValue.startsWith(u"charset="_s)) {
            qsizetype spaceIndex = formattedValue.indexOf(u' ');
            if(spaceIndex > 0)
                formattedValue = formattedValue.mid(spaceIndex + 1);
        }

        exifTags.try_emplace(displayKey, std::move(formattedValue));
    }

    if(exifTags.isEmpty()) {

        QSize size = reader.size();

        if(size.isValid()) {
            exifTags.insert(
                QObject::tr("Dimensions"),
                QString("%1 x %2").arg(size.width()).arg(size.height())
            );
        }
    }
}

const QHash<QString, QString>& DocumentInfo::getExifTags() const {
    if(!exifLoaded)
        loadExifTags();

    return exifTags;
}