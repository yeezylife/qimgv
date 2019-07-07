#include "imagestatic.h"
#include <time.h>

ImageStatic::ImageStatic(QString _path)
    : Image(_path)
{
    load();
}

ImageStatic::ImageStatic(std::unique_ptr<DocumentInfo> _info)
    : Image(std::move(_info))
{
    load();
}

ImageStatic::~ImageStatic() {
}

//load image data from disk
void ImageStatic::load() {
    if(isLoaded()) {
        return;
    }
    std::unique_ptr<const QImage> img(new QImage(mPath, mDocInfo->extension()));
    img = ImageLib::exifRotated(std::move(img), this->mDocInfo.get()->exifOrientation());
    // set image
    image = std::move(img);
    mLoaded = true;
}

// TODO: add a way to configure compression level?
bool ImageStatic::save(QString destPath) {
    // png compression note from libpng
    // Note that tests have shown that zlib compression levels 3-6 usually perform as well
    // as level 9 for PNG images, and do considerably fewer caclulations
    int quality = destPath.endsWith(".png", Qt::CaseInsensitive) ? 30 : 95;
    bool success = false;
    if(isEdited()) {
        success = imageEdited->save(destPath, nullptr, quality);
        image.swap(imageEdited);
        discardEditedImage();
    } else {
        success = image->save(destPath, nullptr, quality);
    }
    if(destPath == mPath && success) {
        mDocInfo->refresh();
    }
    return success;
}

bool ImageStatic::save() {
    return save(mPath);
}

std::unique_ptr<QPixmap> ImageStatic::getPixmap() {
    std::unique_ptr<QPixmap> pix(new QPixmap());
    isEdited()?pix->convertFromImage(*imageEdited):pix->convertFromImage(*image);
    return pix;
}

std::shared_ptr<const QImage> ImageStatic::getSourceImage() {
    return image;
}

std::shared_ptr<const QImage> ImageStatic::getImage() {
    return isEdited()?imageEdited:image;
}

int ImageStatic::height() {
    return isEdited()?imageEdited->height():image->height();
}

int ImageStatic::width() {
    return isEdited()?imageEdited->width():image->width();
}

QSize ImageStatic::size() {
    return isEdited()?imageEdited->size():image->size();
}

bool ImageStatic::setEditedImage(std::unique_ptr<const QImage> imageEditedNew) {
    if(imageEditedNew && imageEditedNew->width() != 0) {
        discardEditedImage();
        imageEdited = std::move(imageEditedNew);
        mEdited = true;
        return true;
    }
    return false;
}

bool ImageStatic::discardEditedImage() {
    if(imageEdited) {
        imageEdited.reset();
        mEdited = false;
        return true;
    }
    return false;
}
