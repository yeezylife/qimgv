#include "documentinfo.h"
#include <QSet>

using namespace Qt::StringLiterals;

// ====================== header cache ======================

const QByteArray& DocumentInfo::headerData(qint64) const {
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

DocumentInfo::DocumentInfo(const QString &path) {
    fileInfo.setFile(path);

    if(!fileInfo.isFile()) {
        qDebug() << "FileInfo: cannot open:" << path;
        return;
    }

    detectFormat();
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
int DocumentInfo::exifOrientation() const { return mOrientation; }

void DocumentInfo::refresh() { fileInfo.refresh(); }

// ====================== detect ======================

void DocumentInfo::detectFormat() {

    if(mDocumentType != NONE)
        return;

    static QMimeDatabase mimeDb;

    mMimeType = mimeDb.mimeTypeForFile(fileInfo, QMimeDatabase::MatchDefault);

    const QByteArray mimeName = mMimeType.name().toLatin1();
    const QByteArray suffix = fileInfo.suffix().toLower().toLatin1();

    if(mimeName == "image/jpeg") {

        mFormat = "jpg";
        mDocumentType = STATIC;

    } else if(mimeName == "image/png") {

        if(QImageReader::supportedImageFormats().contains("apng") && detectAPNG()) {
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

        if(mDocumentType == ANIMATED && !settings->jxlAnimation()) {
            mDocumentType = NONE;
            qDebug() << "animated jxl disabled";
        }

    } else if(mimeName == "image/avif") {

        mFormat = "avif";
        mDocumentType = detectAnimatedAvif() ? ANIMATED : STATIC;

    } else if(mimeName == "image/bmp") {

        mFormat = "bmp";
        mDocumentType = STATIC;

    } else if(settings->videoPlayback() && settings->videoFormats().contains(mimeName)) {

        mDocumentType = VIDEO;
        mFormat = settings->videoFormats().value(mimeName);

    } else {

        mFormat = suffix;

        if(QStringView(mFormat).compare(u"jfif", Qt::CaseInsensitive) == 0)
            mFormat = "jpg";

        if(settings->videoPlayback()) {

            static const QSet<QByteArray> videoSuffixes = [](){
                QSet<QByteArray> set;
                const auto formats = settings->videoFormats().values();
                set.reserve(formats.size());
                for(const auto &fmt : formats)
                    set.insert(fmt);
                return set;
            }();

            if(videoSuffixes.contains(suffix))
                mDocumentType = VIDEO;
            else
                mDocumentType = STATIC;

        } else {
            mDocumentType = STATIC;
        }
    }

    loadExifOrientation();
}

// ====================== detect impl ======================

bool DocumentInfo::detectAPNG() {
    return headerData().contains("acTL");
}

bool DocumentInfo::detectAnimatedWebP() {
    const QByteArray& buf = headerData();

    if (buf.size() < 21)
        return false;

    if (std::memcmp(buf.constData() + 12, "VP8X", 4) != 0)
        return false;

    return buf[20] & 0x02;
}

bool DocumentInfo::detectAnimatedJxl() {
    return false; // 已内联处理
}

bool DocumentInfo::detectAnimatedAvif() {
    const QByteArray& buf = headerData();

    if (buf.size() < 12)
        return false;

    return std::memcmp(buf.constData() + 4, "ftypavis", 8) == 0;
}

// ====================== exif ======================

int DocumentInfo::transformationToExifOrientation(QImageIOHandler::Transformations t) const {

    if (t == QImageIOHandler::TransformationNone) return 1;
    if (t == QImageIOHandler::TransformationRotate180) return 3;
    if (t == QImageIOHandler::TransformationRotate90) return 6;
    if (t == QImageIOHandler::TransformationRotate270) return 8;

    if (t == QImageIOHandler::TransformationMirror) return 2;
    if (t == QImageIOHandler::TransformationFlip) return 4;
    if (t == (QImageIOHandler::TransformationMirror | QImageIOHandler::TransformationRotate270)) return 5;
    if (t == (QImageIOHandler::TransformationMirror | QImageIOHandler::TransformationRotate90)) return 7;

    return 1;
}

void DocumentInfo::loadExifOrientation() {

    if(mDocumentType == VIDEO || mDocumentType == NONE)
        return;

    QImageReader reader(fileInfo.absoluteFilePath());

    if(reader.canRead()) {
        mOrientation = transformationToExifOrientation(reader.transformation());
    }
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

void DocumentInfo::loadExifTags() const {

    if(exifLoaded)
        return;

    exifLoaded = true;
    exifTags.clear();

    QImageReader reader(fileInfo.absoluteFilePath());

    if(!reader.canRead())
        return;

    const auto &mapping = getKeyMapping();
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