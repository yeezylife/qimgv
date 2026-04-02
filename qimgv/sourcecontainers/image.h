#pragma once

#include <QString>
#include <QDebug>
#include <QPixmap>
#include <memory>
#include <QObject>
#include "utils/imagelib.h"
#include "utils/stuff.h"
#include "sourcecontainers/documentinfo.h"

class Image : public QObject {
    Q_OBJECT

public:
    // 使用 explicit 防止隐式转换，使用传值+移动优化 QString 传递
    explicit Image(QString path);
    explicit Image(std::unique_ptr<DocumentInfo> info);
    
    // 【修改1】析构函数简化：使用 default，不再需要纯虚声明和类外定义
    // 既然类中已有其他纯虚函数，类本身就是抽象的，无需通过析构函数强制抽象
    virtual ~Image() = default;

    // --- 内联简单 getter 函数 ---
    QString filePath() const { return mPath; }
    bool isLoaded() const { return mLoaded; }
    bool isEdited() const { return mEdited; }
    
    DocumentType type() const { 
        return mDocInfo ? mDocInfo->type() : DocumentType::NONE; 
    }
    
    QString fileName() const { 
        return mDocInfo ? mDocInfo->fileName() : QString(); 
    }
    
    QString baseName() const { 
        return mDocInfo ? mDocInfo->baseName() : QString(); 
    }
    
    qint64 fileSize() const { 
        return mDocInfo ? mDocInfo->fileSize() : 0; 
    }
    
    QDateTime lastModified() const { 
        return mDocInfo ? mDocInfo->lastModified() : QDateTime(); 
    }
    
   const QHash<QString, QString>& getExifTags() const { 
         static const QHash<QString, QString> empty;
         return mDocInfo ? mDocInfo->getExifTags() : empty; 
     }

    // --- 纯虚函数接口 ---
    virtual void getPixmap(QPixmap& outPixmap) const = 0;
    virtual std::shared_ptr<const QImage> getImage() const = 0;
    virtual int height() const = 0;
    virtual int width() const = 0;
    virtual QSize size() const = 0;
    virtual bool save() = 0;
    virtual bool save(QString destPath) = 0;

protected:
    virtual void load() = 0;
    void loadImage() { load(); }  // 非虚函数包装器

    // 成员变量
    std::unique_ptr<DocumentInfo> mDocInfo;
    QString mPath;
    QSize resolution;

    // 【修改2】成员变量默认初始化
    bool mLoaded = false;
    bool mEdited = false;
};
