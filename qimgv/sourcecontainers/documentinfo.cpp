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
    // 清除缓存的 EXIF 信息，确保下次调用 getExifTags() 时重新加载
    // 这修复了编辑并保存图片后，中文标题等元数据显示为乱码的问题
    exifLoaded = false;
    exifTags.clear();
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

// 新增：QImageIOHandler::Transformations 转换为标准 EXIF Orientation (1-8)
int DocumentInfo::transformationToExifOrientation(QImageIOHandler::Transformations transformation) const {
    // EXIF Orientation 标准值：
    // 1 = 正常
    // 2 = 水平翻转
    // 3 = 旋转180度
    // 4 = 垂直翻转
    // 5 = 水平翻转 + 逆时针旋转90度
    // 6 = 顺时针旋转90度
    // 7 = 水平翻转 + 顺时针旋转90度
    // 8 = 逆时针旋转90度
    
    if (transformation == QImageIOHandler::TransformationNone) {
        return 1; // 正常
    }
    
    bool mirror = transformation & QImageIOHandler::TransformationMirror;
    bool flip = transformation & QImageIOHandler::TransformationFlip;
    bool rotate90 = transformation & QImageIOHandler::TransformationRotate90;
    bool rotate180 = transformation & QImageIOHandler::TransformationRotate180;
    bool rotate270 = transformation & QImageIOHandler::TransformationRotate270;
    
    // 根据组合判断 EXIF orientation
    if (rotate180 && !mirror && !flip) {
        return 3; // 旋转180度
    } else if (rotate90 && !mirror && !flip) {
        return 6; // 顺时针旋转90度
    } else if (rotate270 && !mirror && !flip) {
        return 8; // 逆时针旋转90度（顺时针270度）
    } else if (mirror && !rotate90 && !rotate180 && !rotate270) {
        return 2; // 水平翻转
    } else if (flip && !rotate90 && !rotate180 && !rotate270) {
        return 4; // 垂直翻转
    } else if (mirror && rotate270) {
        return 5; // 水平翻转 + 逆时针旋转90度
    } else if (mirror && rotate90) {
        return 7; // 水平翻转 + 顺时针旋转90度
    }
    
    return 1; // 默认正常
}

void DocumentInfo::loadExifOrientation() {
    if(mDocumentType == DocumentType::VIDEO || mDocumentType == DocumentType::NONE)
        return;

    QString path = filePath();
    // 直接在栈上创建 QImageReader，使用 QStringView 避免临时字符串创建
    QImageReader reader(path, mFormat.toUtf8().constData());

    if(reader.canRead()) {
        QImageIOHandler::Transformations transformation = reader.transformation();
        mOrientation = transformationToExifOrientation(transformation);
    }
    // reader 离开作用域自动析构，无需手动 delete
}

// 新增：格式化元数据值
QString DocumentInfo::formatMetadataValue(const QString &key, const QVariant &value) const {
    // 处理特殊的元数据键
    if (key == "ExposureTime") {
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
    } else if (key == "FNumber" || key == "ApertureValue") {
        bool ok;
        double fnum = value.toDouble(&ok);
        if (ok && fnum > 0) {
            return QString("f/%1").arg(fnum, 0, 'g', 3);
        }
    } else if (key == "FocalLength") {
        bool ok;
        double focal = value.toDouble(&ok);
        if (ok && focal > 0) {
            return QString("%1 %2").arg(focal, 0, 'g', 3).arg(QObject::tr("mm"));
        }
    } else if (key == "ISOSpeedRatings" || key == "PhotographicSensitivity") {
        return value.toString();
    }
    
    return value.toString();
}

void DocumentInfo::loadExifTags() {
    if(exifLoaded)
        return;
    exifLoaded = true;
    exifTags.clear();
    
    QString path = filePath();
    // 必须指定格式，避免在 Windows 上读取元数据时编码错误
    // 特别是当文件名或元数据包含中文时，必须使用正确的格式
    QImageReader reader(path, mFormat.toUtf8().constData());
    
    if (!reader.canRead()) {
        qDebug() << "Cannot read image metadata from:" << path;
        return;
    }
    
    // QImageReader 支持的元数据键名映射
    // 参考：https://doc.qt.io/qt-6/qimagereader.html#textKeys
    
    QStringList textKeys = reader.textKeys();
    
    // 标准 EXIF 键名映射表
    QMap<QString, QString> keyMapping = {
        // 基本信息
        {"Make", QObject::tr("Make")},
        {"Model", QObject::tr("Model")},
        {"DateTime", QObject::tr("Date/Time")},
        {"DateTimeOriginal", QObject::tr("Date/Time")},
        {"DateTimeDigitized", QObject::tr("Date/Time")},
        
        // 曝光信息
        {"ExposureTime", QObject::tr("ExposureTime")},
        {"FNumber", QObject::tr("F Number")},
        {"ApertureValue", QObject::tr("F Number")},
        {"ISOSpeedRatings", QObject::tr("ISO Speed ratings")},
        {"PhotographicSensitivity", QObject::tr("ISO Speed ratings")},
        {"Flash", QObject::tr("Flash")},
        {"FocalLength", QObject::tr("Focal Length")},
        
        // 其他
        {"UserComment", QObject::tr("UserComment")},
        {"ImageDescription", QObject::tr("UserComment")},
        {"Software", "Software"},
        {"Artist", "Artist"},
        {"Copyright", "Copyright"},
        
        // PNG 特定
        {"Description", QObject::tr("UserComment")},
        {"Comment", QObject::tr("UserComment")},
        {"Author", "Artist"},
    };
    
    // 遍历所有可用的文本键
    for (const QString &key : textKeys) {
        QString value = reader.text(key);
        
        if (value.isEmpty())
            continue;
        
        // 查找映射的显示名称
        QString displayKey = keyMapping.value(key, key);
        
        // 格式化特殊值
        QString formattedValue = formatMetadataValue(key, value);
        
        // 处理 UserComment 的特殊情况（去除 charset 前缀）
        if (key == "UserComment" && formattedValue.startsWith("charset=")) {
            int spaceIndex = formattedValue.indexOf(" ");
            if (spaceIndex > 0) {
                formattedValue = formattedValue.mid(spaceIndex + 1);
            }
        }
        
        // 避免重复添加相同的信息
        if (!exifTags.contains(displayKey)) {
            exifTags.insert(displayKey, formattedValue);
        }
    }
    
    // 如果没有找到任何元数据，尝试读取图片尺寸作为基本信息
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
