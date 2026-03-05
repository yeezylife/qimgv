#include "documentinfo.h"

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

// bytes
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

// For cases like orientation / even mimetype change we just reload
// Image from scratch, so don`t bother handling it here
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
        // just try to open via suffix if all of the above fails
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
// dumb apng detector
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

// TODO avoid creating multiple QImageReader instances
bool DocumentInfo::detectAnimatedJxl() {
    QImageReader r(fileInfo.filePath(), "jxl");
    return r.supportsAnimation();
}

bool DocumentInfo::detectAnimatedAvif() {
    QFile f(fileInfo.filePath());
    bool result = false;
    if(f.open(QFile::ReadOnly)) {
        QDataStream in(&f);
        in.skipRawData(4); // skip box size
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

void DocumentInfo::loadExifTags() {
    if(exifLoaded) return;
    exifLoaded = true;
    exifTags.clear();

    QImageReader reader(filePath());
    if(!reader.canRead()) return;

    // 使用字符串键名直接访问 EXIF 元数据
    QStringList keys = { "Make", "Model", "DateTime", "ExposureTime", "FNumber", "ISOSpeedRatings", "Flash", "FocalLength", "UserComment" };
    
    for(const QString &key : keys) {
        QVariant val = reader.metaData(key);
        if(!val.isNull()) {
            exifTags.insert(key, val.toString());
        }
    }
    
    // 特殊处理曝光时间格式
    if(exifTags.contains("ExposureTime")) {
        QString expStr = exifTags.value("ExposureTime");
        qreal exp = expStr.toDouble();
        if(exp > 0 && exp < 1.0) {
            qreal expValue = round(1.0 / exp);
            exifTags.insert("ExposureTime", "1/" + QString::number(expValue) + " sec");
        } else {
            exifTags.insert("ExposureTime", QString::number(exp) + " sec");
        }
    }
    
    // 特殊处理 FNumber 格式
    if(exifTags.contains("FNumber")) {
        QString fStr = exifTags.value("FNumber");
        qreal fn = fStr.toDouble();
        exifTags.insert("FNumber", "f/" + QString::number(fn, 'g', 3));
    }
    
    // 特殊处理 FocalLength 格式
    if(exifTags.contains("FocalLength")) {
        QString flStr = exifTags.value("FocalLength");
        qreal fl = flStr.toDouble();
        exifTags.insert("FocalLength", QString::number(fl, 'g', 3) + " mm");
    }
    
    // 特殊处理 UserComment，移除 charset 信息
    if(exifTags.contains("UserComment")) {
        QString comment = exifTags.value("UserComment");
        if(comment.startsWith("charset=")) {
            comment.remove(0, comment.indexOf(" ") + 1);
            exifTags.insert("UserComment", comment);
        }
    }
}

QMap<QString, QString> DocumentInfo::getExifTags() {
    if(!exifLoaded)
        loadExifTags();
    return exifTags;
}

void DocumentInfo::loadExifOrientation() {
    if(mDocumentType == DocumentType::VIDEO || mDocumentType == DocumentType::NONE)
        return;

    // 直接在栈上构造，无需 new/delete
    QImageReader reader(filePath()); 
    if(!mFormat.isEmpty())
        reader.setFormat(mFormat.toUtf8());

    if(reader.canRead()) {
        // 获取 Qt 风格的变换枚举并转换为标准 EXIF Orientation (1-8)
        auto transform = reader.transformation();
        switch(transform) {
            case QImageIOHandler::TransformationNone: mOrientation = 1; break;
            case QImageIOHandler::TransformationRotate90: mOrientation = 6; break;
            case QImageIOHandler::TransformationRotate180: mOrientation = 3; break;
            case QImageIOHandler::TransformationRotate270: mOrientation = 8; break;
            case QImageIOHandler::TransformationFlipHorizontal: mOrientation = 2; break;
            case QImageIOHandler::TransformationFlipVertical: mOrientation = 4; break;
            case QImageIOHandler::TransformationFlipAboutDiagonal: mOrientation = 5; break;
            case QImageIOHandler::TransformationMirror: mOrientation = 7; break;
            default: mOrientation = 1; break;
        }
    }
}
