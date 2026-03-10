#include "documentinfo.h"

// 使用 Qt 6 推荐的字符串字面量命名空间
using namespace Qt::StringLiterals;

// ##############################################################
// ##################### STATIC METHOD ##########################
// ##############################################################

// 静态函数返回映射表，解决翻译问题
// 使用函数内静态变量实现延迟初始化，确保 QTranslator 已加载
const QMap<QString, QString>& DocumentInfo::getKeyMapping() {
    static const QMap<QString, QString> mapping = {
        // 基本信息
        {u"Make"_s,              QObject::tr("Make")},
        {u"Model"_s,             QObject::tr("Model")},
        {u"DateTime"_s,          QObject::tr("Date/Time")},
        {u"DateTimeOriginal"_s,  QObject::tr("Date/Time")},
        {u"DateTimeDigitized"_s, QObject::tr("Date/Time")},
        
        // 曝光信息
        {u"ExposureTime"_s,      QObject::tr("ExposureTime")},
        {u"FNumber"_s,           QObject::tr("F Number")},
        {u"ApertureValue"_s,     QObject::tr("F Number")},
        {u"ISOSpeedRatings"_s,   QObject::tr("ISO Speed ratings")},
        {u"PhotographicSensitivity"_s, QObject::tr("ISO Speed ratings")},
        {u"Flash"_s,             QObject::tr("Flash")},
        {u"FocalLength"_s,       QObject::tr("Focal Length")},
        
        // 其他
        {u"UserComment"_s,       QObject::tr("UserComment")},
        {u"ImageDescription"_s,  QObject::tr("UserComment")},
        {u"Software"_s,          u"Software"_s},
        {u"Artist"_s,            u"Artist"_s},
        {u"Copyright"_s,         u"Copyright"_s},
        
        // PNG 特定
        {u"Description"_s,       QObject::tr("UserComment")},
        {u"Comment"_s,           QObject::tr("UserComment")},
        {u"Author"_s,            u"Artist"_s},
    };
    return mapping;
}

// ##############################################################
// ####################### CONSTRUCTOR ##########################
// ##############################################################

DocumentInfo::DocumentInfo(QString path)
    : mDocumentType(DocumentType::NONE),
      mOrientation(0),
      mFormat(""),
      exifLoaded(false)
{
    fileInfo.setFile(path);
    if(!fileInfo.isFile()) {
        qDebug() << "FileInfo: cannot open: " << path;
        return;
    }
    detectFormat();
}

DocumentInfo::~DocumentInfo() {
}

// ##############################################################
// ####################### PUBLIC METHODS #######################
// ##############################################################

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

// ##############################################################
// ####################### PRIVATE METHODS ######################
// ##############################################################

void DocumentInfo::detectFormat() {
    if(mDocumentType != DocumentType::NONE)
        return;
    QMimeDatabase mimeDb;
    mMimeType = mimeDb.mimeTypeForFile(fileInfo.filePath(), QMimeDatabase::MatchContent);
    auto mimeName = mMimeType.name().toUtf8();
    auto suffix = fileInfo.suffix().toLower().toUtf8();
    
    if(mimeName == "image/jpeg") {
        mFormat = "jpg";
        mDocumentType = DocumentType::STATIC;
    } else if(mimeName == "image/png") {
        if(QImageReader::supportedImageFormats().contains("apng") && detectAPNG()) {
            mFormat = "apng";
            mDocumentType = DocumentType::ANIMATED;
        } else {
            mFormat = "png";
            mDocumentType = DocumentType::STATIC;
        }
    } else if(mimeName == "image/gif") {
        mFormat = "gif";
        mDocumentType = DocumentType::ANIMATED;
    } else if(mimeName == "image/webp" || (mimeName == "audio/x-riff" && suffix == "webp")) {
        mFormat = "webp";
        mDocumentType = detectAnimatedWebP() ? DocumentType::ANIMATED : DocumentType::STATIC;
    } else if(mimeName == "image/jxl") {
        mFormat = "jxl";
        mDocumentType = detectAnimatedJxl() ? DocumentType::ANIMATED : DocumentType::STATIC;
        if(mDocumentType == DocumentType::ANIMATED && !settings->jxlAnimation()) {
            mDocumentType = DocumentType::NONE;
            qDebug() << "animated jxl is off; skipping file";
        }
    } else if(mimeName == "image/avif") {
        mFormat = "avif";
        mDocumentType = detectAnimatedAvif() ? DocumentType::ANIMATED : DocumentType::STATIC;
    } else if(mimeName == "image/bmp") {
        mFormat = "bmp";
        mDocumentType = DocumentType::STATIC;
    } else if(settings->videoPlayback() && settings->videoFormats().contains(mimeName)) {
        mDocumentType = DocumentType::VIDEO;
        mFormat = settings->videoFormats().value(mimeName);
    } else {
        mFormat = suffix;
        if(QStringView(mFormat).compare(u"jfif", Qt::CaseInsensitive) == 0)
            mFormat = "jpg";
        if(settings->videoPlayback() && settings->videoFormats().values().contains(suffix))
            mDocumentType = DocumentType::VIDEO;
        else
            mDocumentType = DocumentType::STATIC;
    }
    loadExifOrientation();
}

inline
bool DocumentInfo::detectAPNG() {
    QFile f(fileInfo.filePath());
    if(f.open(QFile::ReadOnly)) {
        QDataStream in(&f);
        const int len = 120;
        QByteArray qbuf("\0", len);
        if (in.readRawData(qbuf.data(), len) > 0) {
            return qbuf.contains("acTL");
        }
    }
    return false;
}

bool DocumentInfo::detectAnimatedWebP() {
    QFile f(fileInfo.filePath());
    bool result = false;
    if(f.open(QFile::ReadOnly)) {
        QDataStream in(&f);
        in.skipRawData(12);
        char *buf = static_cast<char*>(malloc(5));
        buf[4] = '\0';
        in.readRawData(buf, 4);
        if(strcmp(buf, "VP8X") == 0) {
            in.skipRawData(4);
            char flags;
            in.readRawData(&flags, 1);
            if(flags & (1 << 1)) {
                result = true;
            }
        }
        free(buf);
    }
    return result;
}

bool DocumentInfo::detectAnimatedJxl() {
    QImageReader r(fileInfo.filePath(), "jxl");
    return r.supportsAnimation();
}

bool DocumentInfo::detectAnimatedAvif() {
    QFile f(fileInfo.filePath());
    bool result = false;
    if(f.open(QFile::ReadOnly)) {
        QDataStream in(&f);
        in.skipRawData(4);
        char *buf = static_cast<char*>(malloc(9));
        buf[8] = '\0';
        in.readRawData(buf, 8);
        if(strcmp(buf, "ftypavis") == 0) {
            result = true;
        }
        free(buf);
    }
    return result;
}

int DocumentInfo::transformationToExifOrientation(QImageIOHandler::Transformations transformation) const {
    if (transformation == QImageIOHandler::TransformationNone) {
        return 1;
    }
    
    bool mirror = transformation & QImageIOHandler::TransformationMirror;
    bool flip = transformation & QImageIOHandler::TransformationFlip;
    bool rotate90 = transformation & QImageIOHandler::TransformationRotate90;
    bool rotate180 = transformation & QImageIOHandler::TransformationRotate180;
    bool rotate270 = transformation & QImageIOHandler::TransformationRotate270;
    
    if (rotate180 && !mirror && !flip) {
        return 3;
    } else if (rotate90 && !mirror && !flip) {
        return 6;
    } else if (rotate270 && !mirror && !flip) {
        return 8;
    } else if (mirror && !rotate90 && !rotate180 && !rotate270) {
        return 2;
    } else if (flip && !rotate90 && !rotate180 && !rotate270) {
        return 4;
    } else if (mirror && rotate270) {
        return 5;
    } else if (mirror && rotate90) {
        return 7;
    }
    
    return 1;
}

void DocumentInfo::loadExifOrientation() {
    if(mDocumentType == DocumentType::VIDEO || mDocumentType == DocumentType::NONE)
        return;

    QString path = filePath();
    QByteArray formatBytes = mFormat.toUtf8();
    QImageReader reader(path, formatBytes.constData());

    if(reader.canRead()) {
        QImageIOHandler::Transformations transformation = reader.transformation();
        mOrientation = transformationToExifOrientation(transformation);
    }
}

QString DocumentInfo::formatMetadataValue(const QString &key, const QVariant &value) const {
    if (key == u"ExposureTime"_s) {
        bool ok;
        double expTime = value.toDouble(&ok);
        if (ok && expTime > 0) {
            if (expTime < 1.0) {
                int denominator = qRound(1.0 / expTime);
                return QString("1/%1 %2").arg(denominator).arg(QObject::tr("sec"));
            } else {
                return QString("%1 %2").arg(expTime, 0, 'f', 2).arg(QObject::tr("sec"));
            }
        }
    } else if (key == u"FNumber"_s || key == u"ApertureValue"_s) {
        bool ok;
        double fnum = value.toDouble(&ok);
        if (ok && fnum > 0) {
            return QString("f/%1").arg(fnum, 0, 'g', 3);
        }
    } else if (key == u"FocalLength"_s) {
        bool ok;
        double focal = value.toDouble(&ok);
        if (ok && focal > 0) {
            return QString("%1 %2").arg(focal, 0, 'g', 3).arg(QObject::tr("mm"));
        }
    }
    return value.toString();
}

void DocumentInfo::loadExifTags() {
    if(exifLoaded)
        return;
    exifLoaded = true;
    exifTags.clear();
    
    QString path = filePath();
    QByteArray formatBytes = mFormat.toUtf8();
    QImageReader reader(path, formatBytes.constData());
    
    if (!reader.canRead()) {
        return;
    }
    
    // 获取静态映射表
    const auto& mapping = getKeyMapping();
    QStringList textKeys = reader.textKeys();
    
    for (const QString &key : textKeys) {
        QString value = reader.text(key);
        
        if (value.isEmpty())
            continue;
        
        QString displayKey = mapping.value(key, key);
        QString formattedValue = formatMetadataValue(key, value);
        
        if (key == u"UserComment"_s && formattedValue.startsWith(u"charset="_s)) {
            int spaceIndex = formattedValue.indexOf(u" "_s);
            if (spaceIndex > 0) {
                formattedValue = formattedValue.mid(spaceIndex + 1);
            }
        }
        
        if (!exifTags.contains(displayKey)) {
            exifTags.insert(displayKey, formattedValue);
        }
    }
    
    if (exifTags.isEmpty()) {
        QSize size = reader.size();
        if (size.isValid()) {
            exifTags.insert(QObject::tr("Dimensions"), 
                          QString("%1 x %2").arg(size.width()).arg(size.height()));
        }
    }
}

QMap<QString, QString> DocumentInfo::getExifTags() {
    if(!exifLoaded)
        loadExifTags();
    return exifTags;
}
