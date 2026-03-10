#include "documentinfo.h"

using namespace Qt::StringLiterals;

const QMap<QString, QString>& DocumentInfo::getKeyMapping() {
    static const QMap<QString, QString> mapping = {

        {u"Make"_s, QObject::tr("Make")},
        {u"Model"_s, QObject::tr("Model")},

        {u"DateTime"_s, QObject::tr("Date/Time")},
        {u"DateTimeOriginal"_s, QObject::tr("Date/Time")},
        {u"DateTimeDigitized"_s, QObject::tr("Date/Time")},

        {u"ExposureTime"_s, QObject::tr("ExposureTime")},
        {u"FNumber"_s, QObject::tr("F Number")},
        {u"ApertureValue"_s, QObject::tr("F Number")},

        {u"ISOSpeedRatings"_s, QObject::tr("ISO Speed ratings")},
        {u"PhotographicSensitivity"_s, QObject::tr("ISO Speed ratings")},

        {u"Flash"_s, QObject::tr("Flash")},
        {u"FocalLength"_s, QObject::tr("Focal Length")},

        {u"UserComment"_s, QObject::tr("UserComment")},
        {u"ImageDescription"_s, QObject::tr("UserComment")},

        {u"Software"_s, u"Software"_s},
        {u"Artist"_s, u"Artist"_s},
        {u"Copyright"_s, u"Copyright"_s},

        {u"Description"_s, QObject::tr("UserComment")},
        {u"Comment"_s, QObject::tr("UserComment")},
        {u"Author"_s, u"Artist"_s},
    };

    return mapping;
}

DocumentInfo::DocumentInfo(const QString &path)
{
    fileInfo.setFile(path);

    if(!fileInfo.isFile()) {
        qDebug() << "FileInfo: cannot open:" << path;
        return;
    }

    detectFormat();
}

DocumentInfo::~DocumentInfo() {}

QString DocumentInfo::directoryPath() const {
    return fileInfo.absolutePath();
}

QString DocumentInfo::filePath() const {
    return fileInfo.absoluteFilePath();
}

QString DocumentInfo::fileName() const {
    return fileInfo.fileName();
}

QString DocumentInfo::baseName() const {
    return fileInfo.baseName();
}

qint64 DocumentInfo::fileSize() const {
    return fileInfo.size();
}

DocumentType DocumentInfo::type() const {
    return mDocumentType;
}

QMimeType DocumentInfo::mimeType() const {
    return mMimeType;
}

QString DocumentInfo::format() const {
    return mFormat;
}

QDateTime DocumentInfo::lastModified() const {
    return fileInfo.lastModified();
}

void DocumentInfo::refresh() {
    fileInfo.refresh();
}

int DocumentInfo::exifOrientation() const {
    return mOrientation;
}

void DocumentInfo::detectFormat() {

    if(mDocumentType != NONE)
        return;

    QMimeDatabase mimeDb;

    mMimeType = mimeDb.mimeTypeForFile(fileInfo.filePath(), QMimeDatabase::MatchContent);

    QByteArray mimeName = mMimeType.name().toUtf8();
    QByteArray suffix = fileInfo.suffix().toLower().toUtf8();

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
        mDocumentType = detectAnimatedJxl() ? ANIMATED : STATIC;

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

        if(settings->videoPlayback() && settings->videoFormats().values().contains(suffix))
            mDocumentType = VIDEO;
        else
            mDocumentType = STATIC;
    }

    loadExifOrientation();
}

bool DocumentInfo::detectAPNG() {

    QFile f(fileInfo.filePath());

    if(!f.open(QFile::ReadOnly))
        return false;

    QByteArray buf = f.read(120);

    return buf.contains("acTL");
}

bool DocumentInfo::detectAnimatedWebP() {

    QFile f(fileInfo.filePath());

    if(!f.open(QFile::ReadOnly))
        return false;

    QDataStream in(&f);

    in.skipRawData(12);

    char header[5] = {0};

    if(in.readRawData(header,4) != 4)
        return false;

    if(strcmp(header,"VP8X") != 0)
        return false;

    in.skipRawData(4);

    char flags;

    if(in.readRawData(&flags,1) != 1)
        return false;

    return flags & (1<<1);
}

bool DocumentInfo::detectAnimatedJxl() {

    QImageReader r(fileInfo.filePath(), "jxl");

    return r.supportsAnimation();
}

bool DocumentInfo::detectAnimatedAvif() {

    QFile f(fileInfo.filePath());

    if(!f.open(QFile::ReadOnly))
        return false;

    QDataStream in(&f);

    in.skipRawData(4);

    char buf[9] = {0};

    if(in.readRawData(buf,8) != 8)
        return false;

    return strcmp(buf,"ftypavis") == 0;
}

int DocumentInfo::transformationToExifOrientation(QImageIOHandler::Transformations transformation) const {

    if(transformation == QImageIOHandler::TransformationNone)
        return 1;

    bool mirror = transformation & QImageIOHandler::TransformationMirror;
    bool flip = transformation & QImageIOHandler::TransformationFlip;
    bool rotate90 = transformation & QImageIOHandler::TransformationRotate90;
    bool rotate180 = transformation & QImageIOHandler::TransformationRotate180;
    bool rotate270 = transformation & QImageIOHandler::TransformationRotate270;

    if(rotate180 && !mirror && !flip) return 3;
    if(rotate90 && !mirror && !flip) return 6;
    if(rotate270 && !mirror && !flip) return 8;
    if(mirror && !rotate90 && !rotate180 && !rotate270) return 2;
    if(flip && !rotate90 && !rotate180 && !rotate270) return 4;
    if(mirror && rotate270) return 5;
    if(mirror && rotate90) return 7;

    return 1;
}

void DocumentInfo::loadExifOrientation() {

    if(mDocumentType == VIDEO || mDocumentType == NONE)
        return;

    QImageReader reader(filePath(), mFormat.toUtf8());

    if(reader.canRead()) {

        auto transformation = reader.transformation();

        mOrientation = transformationToExifOrientation(transformation);
    }
}

QString DocumentInfo::formatMetadataValue(const QString &key,const QVariant &value) const {

    if(key == u"ExposureTime"_s) {

        bool ok;
        double t = value.toDouble(&ok);

        if(ok && t>0) {

            if(t<1.0)
                return QString("1/%1 %2").arg(qRound(1.0/t)).arg(QObject::tr("sec"));

            return QString("%1 %2").arg(t,0,'f',2).arg(QObject::tr("sec"));
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

    QImageReader reader(filePath(), mFormat.toUtf8());

    if(!reader.canRead())
        return;

    const auto &mapping = getKeyMapping();

    QStringList textKeys = reader.textKeys();

    for(const QString &key : textKeys) {

        QString value = reader.text(key);

        if(value.isEmpty())
            continue;

        QString displayKey = mapping.value(key, key);

        QString formattedValue = formatMetadataValue(key, value);

        // EXIF UserComment 特殊处理
        if(key == u"UserComment"_s && formattedValue.startsWith(u"charset="_s)) {

            int spaceIndex = formattedValue.indexOf(u' ');

            if(spaceIndex > 0)
                formattedValue = formattedValue.mid(spaceIndex + 1);
        }

        if(!exifTags.contains(displayKey))
            exifTags.insert(displayKey, formattedValue);
    }

    // 如果没有 EXIF 信息，则至少显示图片尺寸
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

const QMap<QString, QString>& DocumentInfo::getExifTags() const
{
    if(!exifLoaded)
        loadExifTags();

    return exifTags;
}