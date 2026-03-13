#include "cvmatandqimage.h"
#include <opencv2/imgproc.hpp>

namespace QtOcv {
namespace {

/*ARGB <==> BGRA
 */
cv::Mat argb2bgra(const cv::Mat &mat)
{
    Q_ASSERT(mat.channels()==4);

    cv::Mat newMat(mat.rows, mat.cols, mat.type());
    int from_to[] = {0,3, 1,2, 2,1, 3,0};
    cv::mixChannels(&mat, 1, &newMat, 1, from_to, 4);
    return newMat;
}

cv::Mat adjustChannelsOrder(const cv::Mat &srcMat, MatColorOrder srcOrder, MatColorOrder targetOrder)
{
    Q_ASSERT(srcMat.channels()==4);

    if (srcOrder == targetOrder)
        return srcMat.clone();

    cv::Mat desMat;

    if ((srcOrder == MatColorOrder::ARGB && targetOrder == MatColorOrder::BGRA)
            ||(srcOrder == MatColorOrder::BGRA && targetOrder == MatColorOrder::ARGB)) {
        desMat = argb2bgra(srcMat);
    } else if (srcOrder == MatColorOrder::ARGB && targetOrder == MatColorOrder::RGBA) {
        desMat = cv::Mat(srcMat.rows, srcMat.cols, srcMat.type());
        int from_to[] = {0,3, 1,0, 2,1, 3,2};
        cv::mixChannels(&srcMat, 1, &desMat, 1, from_to, 4);
    } else if (srcOrder == MatColorOrder::RGBA && targetOrder == MatColorOrder::ARGB) {
        desMat = cv::Mat(srcMat.rows, srcMat.cols, srcMat.type());
        int from_to[] = {0,1, 1,2, 2,3, 3,0};
        cv::mixChannels(&srcMat, 1, &desMat, 1, from_to, 4);
    } else {
        cv::cvtColor(srcMat, desMat, cv::COLOR_BGRA2RGBA);
    }
    return desMat;
}

QImage::Format findClosestFormat(QImage::Format formatHint)
{
    QImage::Format format;
    switch (formatHint) {
    case QImage::Format_Indexed8:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGB888:
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_Alpha8:
    case QImage::Format_Grayscale8:
        format = formatHint;
        break;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        format = QImage::Format_Indexed8;
        break;
    case QImage::Format_RGB16:
        format = QImage::Format_RGB32;
        break;
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB666:
        format = QImage::Format_RGB888;
        break;
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        format = QImage::Format_ARGB32;
        break;
    }
    return format;
}

constexpr MatColorOrder getColorOrderOfRGB32Format() noexcept
{
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return MatColorOrder::BGRA;
#else
        return MatColorOrder::ARGB;
#endif
}

} // namespace


cv::Mat image2Mat(const QImage &img, int requiredMatType, MatColorOrder requriedOrder)
{
    int targetDepth = CV_MAT_DEPTH(requiredMatType);
    int targetChannels = CV_MAT_CN(requiredMatType);

    Q_ASSERT(targetChannels==CV_CN_MAX || targetChannels==1 || targetChannels==3 || targetChannels==4);
    Q_ASSERT(targetDepth==CV_8U || targetDepth==CV_16U || targetDepth==CV_32F);

    if (img.isNull())
        return cv::Mat();

    QImage::Format format = findClosestFormat(img.format());
    QImage image = (format==img.format()) ? img : img.convertToFormat(format);

    MatColorOrder srcOrder = MatColorOrder::BGR; // 防止 analyzer 报未初始化
    cv::Mat mat0 = image2Mat_shared(image, &srcOrder);

    cv::Mat mat_adjustCn;

    const float maxAlpha =
        targetDepth==CV_8U ? 255.0f :
        (targetDepth==CV_16U ? 65535.0f : 1.0f);

    if (targetChannels == CV_CN_MAX)
        targetChannels = mat0.channels();

    switch(targetChannels) {

    case 1:
        if (mat0.channels() == 3) {
            cv::cvtColor(mat0, mat_adjustCn, cv::COLOR_RGB2GRAY);
        } else if (mat0.channels() == 4) {
            if (srcOrder == MatColorOrder::BGRA)
                cv::cvtColor(mat0, mat_adjustCn, cv::COLOR_BGRA2GRAY);
            else if (srcOrder == MatColorOrder::RGBA)
                cv::cvtColor(mat0, mat_adjustCn, cv::COLOR_RGBA2GRAY);
            else
                cv::cvtColor(argb2bgra(mat0), mat_adjustCn, cv::COLOR_BGRA2GRAY);
        }
        break;

    case 3:
        if (mat0.channels() == 1) {
            cv::cvtColor(mat0, mat_adjustCn,
                         requriedOrder == MatColorOrder::BGR ?
                         cv::COLOR_GRAY2BGR : cv::COLOR_GRAY2RGB);
        } else if (mat0.channels() == 3) {
            if (requriedOrder != srcOrder)
                cv::cvtColor(mat0, mat_adjustCn, cv::COLOR_RGB2BGR);
        } else if (mat0.channels() == 4) {

            if (srcOrder == MatColorOrder::ARGB) {
                mat_adjustCn = cv::Mat(mat0.rows, mat0.cols,
                                       CV_MAKE_TYPE(mat0.type(), 3));

                int ARGB2RGB[] = {1,0, 2,1, 3,2};
                int ARGB2BGR[] = {1,2, 2,1, 3,0};

                cv::mixChannels(&mat0, 1, &mat_adjustCn, 1,
                                requriedOrder == MatColorOrder::BGR ?
                                ARGB2BGR : ARGB2RGB, 3);
            }
            else if (srcOrder == MatColorOrder::BGRA) {
                cv::cvtColor(mat0, mat_adjustCn,
                             requriedOrder == MatColorOrder::BGR ?
                             cv::COLOR_BGRA2BGR : cv::COLOR_BGRA2RGB);
            }
            else {
                cv::cvtColor(mat0, mat_adjustCn,
                             requriedOrder == MatColorOrder::BGR ?
                             cv::COLOR_RGBA2BGR : cv::COLOR_RGBA2RGB);
            }
        }
        break;

    case 4:
        if (mat0.channels() == 4 && srcOrder != requriedOrder)
            mat_adjustCn = adjustChannelsOrder(mat0, srcOrder, requriedOrder);
        break;

    default:
        break;
    }

    if (targetDepth == CV_8U)
        return mat_adjustCn.empty() ? mat0.clone() : mat_adjustCn;

    if (mat_adjustCn.empty())
        mat_adjustCn = mat0;

    cv::Mat mat_adjustDepth;

    mat_adjustCn.convertTo(
        mat_adjustDepth,
        CV_MAKE_TYPE(targetDepth, mat_adjustCn.channels()),
        targetDepth == CV_16U ? 255.0 : 1.0/255.0
    );

    return mat_adjustDepth;
}


/* Convert QImage to cv::Mat without data copy */
cv::Mat image2Mat_shared(const QImage &img, MatColorOrder *order)
{
    if (order)
        *order = MatColorOrder::BGR; // 默认值防 analyzer

    if (img.isNull())
        return cv::Mat();

    switch (img.format()) {

    case QImage::Format_RGB888:
        if (order) *order = MatColorOrder::RGB;
        break;

    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        if (order) *order = getColorOrderOfRGB32Format();
        break;

    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
        if (order) *order = MatColorOrder::RGBA;
        break;

    case QImage::Format_Indexed8:
    case QImage::Format_Alpha8:
    case QImage::Format_Grayscale8:
        break;

    default:
        return cv::Mat();
    }

    return cv::Mat(
        img.height(),
        img.width(),
        CV_8UC(img.depth()/8),
        (uchar*)img.bits(),
        img.bytesPerLine()
    );
}


/* Convert  cv::Mat to QImage without data copy */
QImage mat2Image_shared(const cv::Mat &mat, QImage::Format formatHint)
{
    Q_ASSERT(mat.type() == CV_8UC1 || mat.type() == CV_8UC3 || mat.type() == CV_8UC4);

    if (mat.empty())
        return QImage();

    if (mat.type() == CV_8UC1) {
        if (formatHint != QImage::Format_Indexed8 &&
            formatHint != QImage::Format_Alpha8 &&
            formatHint != QImage::Format_Grayscale8)
            formatHint = QImage::Format_Indexed8;
    }
    else if (mat.type() == CV_8UC3) {
        formatHint = QImage::Format_RGB888;
    }
    else if (mat.type() == CV_8UC4) {
        if (formatHint != QImage::Format_RGB32 &&
            formatHint != QImage::Format_ARGB32 &&
            formatHint != QImage::Format_ARGB32_Premultiplied &&
            formatHint != QImage::Format_RGBX8888 &&
            formatHint != QImage::Format_RGBA8888 &&
            formatHint != QImage::Format_RGBA8888_Premultiplied)
            formatHint = QImage::Format_ARGB32;
    }

    QImage img(
        mat.data,
        mat.cols,
        mat.rows,
        static_cast<qsizetype>(mat.step), // 修复 narrowing
        formatHint
    );

    if (formatHint == QImage::Format_Indexed8) {
        QVector<QRgb> colorTable;
        colorTable.reserve(256);
        for (int i=0;i<256;i++)
            colorTable.append(qRgb(i,i,i));
        img.setColorTable(colorTable);
    }

    return img;
}

bool isSupported(QImage::Format format) {
    return format == QImage::Format_RGB888 ||
           format == QImage::Format_Grayscale8 ||
           format == QImage::Format_RGB32 ||
           format == QImage::Format_ARGB32 ||
           format == QImage::Format_ARGB32_Premultiplied;
}

} // namespace QtOcv