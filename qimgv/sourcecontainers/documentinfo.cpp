#include "documentinfo.h"
#include "settings.h"
#include <QDebug>
#include <QImageReader>
#include <QMimeDatabase>
#include <QDataStream>
#include <memory>   // for std::unique_ptr

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
        if(mFormat.compare("jfif", Qt::CaseInsensitive) == 0)
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

// ==================== Exiv2 相关代码 ====================
#ifdef USE_EXIV2
#include <exiv2/exiv2.hpp>
#include <memory>   // for std::unique_ptr
#include <QFile>

/**
 * @brief 自定义 Exiv2::BasicIo 实现，通过 QFile 操作文件。
 * 
 * 完全绕过路径字符串，直接将 Qt 打开的文件句柄提供给 Exiv2，
 * 实现零编码转换，完美支持所有 Unicode 文件名。
 * 
 * 此类实现了所有 BasicIo 接口方法，包括写入操作，因此未来如需
 * 修改 EXIF 信息（例如旋转后保存），可直接使用。
 * 
 * 注意：mmap 方法返回 nullptr，但对于 EXIF 读取/写入（通常只涉及
 * 文件头部小数据块），普通读取效率足够，无需内存映射。
 */
class QtFileIo : public Exiv2::BasicIo {
public:
    explicit QtFileIo(std::unique_ptr<QFile> file)
        : file_(std::move(file))
        , pos_(0)
        , size_(file_->size())
        , isOpen_(file_->isOpen())
        , path_(file_->fileName().toStdString())
    {
        // 移除断言，允许只读打开（写入方法会自行处理）
        // Q_ASSERT(file_->openMode() & QIODevice::ReadWrite);
    }

    QtFileIo(const QtFileIo&) = delete;
    QtFileIo& operator=(const QtFileIo&) = delete;
    ~QtFileIo() override = default;

    int open() override {
        if (isOpen_) {
            pos_ = 0;
            return 0;
        }
        return -1;
    }

    int close() override {
        isOpen_ = false;
        return 0;
    }

    // --- 写入方法 ---
    size_t write(const Exiv2::byte* data, size_t wcount) override {
        if (!isOpen_ || !data || wcount == 0) return 0;
        if (!file_->isWritable()) {
            qWarning() << "QtFileIo: Attempted to write to a read-only file.";
            return 0;
        }
        qint64 bytesWritten = file_->write(reinterpret_cast<const char*>(data), wcount);
        if (bytesWritten > 0) {
            pos_ += bytesWritten;
            if (pos_ > size_) size_ = pos_;
        }
        return static_cast<size_t>(bytesWritten);
    }

    size_t write(Exiv2::BasicIo& src) override {
        if (!isOpen_) return 0;
        const size_t blockSize = 4096;
        Exiv2::byte buffer[blockSize];
        size_t totalWritten = 0;
        while (!src.eof()) {
            size_t n = src.read(buffer, blockSize);
            if (n == 0) break;
            size_t written = write(buffer, n);
            if (written != n) break;
            totalWritten += written;
        }
        return totalWritten;
    }

    int putb(Exiv2::byte data) override {
        return (write(&data, 1) == 1) ? data : EOF;
    }

    // --- 读取方法 ---
    Exiv2::DataBuf read(size_t rcount) override {
        if (!isOpen_ || rcount == 0) return Exiv2::DataBuf();
        size_t bytesToRead = rcount;
        if (pos_ + bytesToRead > size_) bytesToRead = static_cast<size_t>(size_ - pos_);
        if (bytesToRead == 0) return Exiv2::DataBuf();
        Exiv2::DataBuf buf(bytesToRead);
        size_t bytesRead = read(buf.data(), bytesToRead);
        if (bytesRead > 0 && bytesRead != bytesToRead) {
            buf.resize(bytesRead);
        }
        return buf;
    }

    size_t read(Exiv2::byte* buf, size_t rcount) override {
        if (!isOpen_ || !buf || rcount == 0) return 0;
        size_t bytesToRead = rcount;
        if (pos_ + bytesToRead > size_) bytesToRead = static_cast<size_t>(size_ - pos_);
        if (bytesToRead == 0) return 0;
        qint64 bytesRead = file_->read(reinterpret_cast<char*>(buf), bytesToRead);
        if (bytesRead > 0) pos_ += bytesRead;
        return static_cast<size_t>(bytesRead);
    }

    int getb() override {
        if (!isOpen_ || pos_ >= size_) return EOF;
        Exiv2::byte b = 0;
        return (read(&b, 1) == 1) ? b : EOF;
    }

    void transfer(Exiv2::BasicIo& src) override {
        write(src);
    }

    int seek(int64_t offset, Position pos) override {
        if (!isOpen_) return -1;
        int64_t newPos = -1;
        switch (pos) {
            case Exiv2::BasicIo::beg: newPos = offset; break;
            case Exiv2::BasicIo::cur: newPos = pos_ + offset; break;
            case Exiv2::BasicIo::end: newPos = static_cast<int64_t>(size_) + offset; break;
        }
        if (newPos < 0 || newPos > static_cast<int64_t>(size_)) return -1;
        if (file_->seek(newPos)) {
            pos_ = newPos;
            return 0;
        }
        return -1;
    }

    Exiv2::byte* mmap(bool /*isWriteable*/) override { return nullptr; }
    int munmap() override { return 0; }

    size_t tell() const override { return static_cast<size_t>(pos_); }
    size_t size() const override { return static_cast<size_t>(size_); }
    bool isopen() const override { return isOpen_; }
    int error() const override { return file_->error() != QFile::NoError ? 1 : 0; }
    bool eof() const override { return pos_ >= size_; }
    const std::string& path() const override { return path_; }
#ifdef EXV_UNICODE_PATH
    std::wstring wpath() const override { return file_->fileName().toStdWString(); }
#endif

    // 新增：Exiv2 0.28+ 要求实现 populateFakeData（通常空实现即可）
    void populateFakeData() override {}

private:
    std::unique_ptr<QFile> file_;
    qint64 pos_;        // 注意：保持 qint64 以便处理大文件
    qint64 size_;
    bool isOpen_;
    std::string path_;  // 存储 path 字符串以供 path() 返回引用
};

#endif // USE_EXIV2

void DocumentInfo::loadExifTags() {
    if(exifLoaded)
        return;
    exifLoaded = true;
    exifTags.clear();
#ifdef USE_EXIV2
    try {
        std::unique_ptr<Exiv2::Image> image;

        // --- 最健壮的方式：使用 QFile + 自定义 BasicIo ---
        // 1. 用 QFile 打开文件（QFile 完美支持 Unicode 路径）
        auto file = std::make_unique<QFile>(fileInfo.filePath());
        // 优先尝试读写模式（为以后修改做准备），如果失败则尝试只读
        if (!file->open(QIODevice::ReadWrite)) {
            if (!file->open(QIODevice::ReadOnly)) {
                return; // 彻底打不开才返回
            }
        }

        // 2. 将 QFile 的所有权交给 QtFileIo
        auto qtFileIo = std::make_unique<QtFileIo>(std::move(file));

        // 3. 使用 Exiv2::ImageFactory::open 的重载版本，接受 BasicIo 唯一指针
        image = Exiv2::ImageFactory::open(std::move(qtFileIo));

        assert(image.get() != 0);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        if(exifData.empty())
            return;

        // 以下 EXIF 解析代码保持不变
        Exiv2::ExifKey make("Exif.Image.Make");
        Exiv2::ExifKey model("Exif.Image.Model");
        Exiv2::ExifKey dateTime("Exif.Image.DateTime");
        Exiv2::ExifKey exposureTime("Exif.Photo.ExposureTime");
        Exiv2::ExifKey fnumber("Exif.Photo.FNumber");
        Exiv2::ExifKey isoSpeedRatings("Exif.Photo.ISOSpeedRatings");
        Exiv2::ExifKey flash("Exif.Photo.Flash");
        Exiv2::ExifKey focalLength("Exif.Photo.FocalLength");
        Exiv2::ExifKey userComment("Exif.Photo.UserComment");

        Exiv2::ExifData::const_iterator it;

        it = exifData.findKey(make);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("Make"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(model);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("Model"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(dateTime);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("Date/Time"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(exposureTime);
        if(it != exifData.end()) {
            Exiv2::Rational r = it->toRational();
            if(r.first < r.second) {
                qreal exp = round(static_cast<qreal>(r.second) / r.first);
                exifTags.insert(QObject::tr("ExposureTime"), "1/" + QString::number(exp) + QObject::tr(" sec"));
            } else {
                qreal exp = round(static_cast<qreal>(r.first) / r.second);
                exifTags.insert(QObject::tr("ExposureTime"), QString::number(exp) + QObject::tr(" sec"));
            }
        }

        it = exifData.findKey(fnumber);
        if(it != exifData.end()) {
            Exiv2::Rational r = it->toRational();
            qreal fn = static_cast<qreal>(r.first) / r.second;
            exifTags.insert(QObject::tr("F Number"), "f/" + QString::number(fn, 'g', 3));
        }

        it = exifData.findKey(isoSpeedRatings);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("ISO Speed ratings"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(flash);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("Flash"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(focalLength);
        if(it != exifData.end()) {
            Exiv2::Rational r = it->toRational();
            qreal fn = static_cast<qreal>(r.first) / r.second;
            exifTags.insert(QObject::tr("Focal Length"), QString::number(fn, 'g', 3) + QObject::tr(" mm"));
        }

        it = exifData.findKey(userComment);
        if(it != exifData.end()) {
            auto comment = QString::fromStdString(it->value().toString());
            if(comment.startsWith("charset="))
                comment.remove(0, comment.indexOf(" ") + 1);
            exifTags.insert(QObject::tr("UserComment"), comment);
        }
    }

#if not EXIV2_TEST_VERSION(0, 28, 0)
#ifdef __WIN32
    catch (Exiv2::BasicError<wchar_t>& e) {
        qDebug() << "Caught Exiv2::BasicError exception:\n" << e.what() << "\n";
        return;
    }
#else
    catch (Exiv2::BasicError<char>& e) {
        qDebug() << "Caught Exiv2::BasicError exception:\n" << e.what() << "\n";
        return;
    }
#endif
#endif
    catch (Exiv2::Error& e) {
        qDebug() << "Caught Exiv2 exception:\n" << e.what() << "\n";
        return;
    }
#endif
}

QMap<QString, QString> DocumentInfo::getExifTags() {
    if(!exifLoaded)
        loadExifTags();
    return exifTags;
}

void DocumentInfo::loadExifOrientation() {
    if(mDocumentType == DocumentType::VIDEO || mDocumentType == DocumentType::NONE)
        return;

    QString path = filePath();
    QImageReader reader(path, mFormat.isEmpty() ? nullptr : mFormat.toStdString().c_str());

    if(reader.canRead())
        mOrientation = static_cast<int>(reader.transformation());
}
