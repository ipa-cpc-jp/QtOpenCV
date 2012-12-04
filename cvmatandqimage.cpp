/****************************************************************************
** Copyright (c) 2012 Debao Zhang <hello@debao.me>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/

#include "cvmatandqimage.h"
#include <QImage>
#include <cstring>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

namespace QtOcv {

/* Convert QImage to cv::Mat
 *
 * - Supported dest mat channels is 0, 1, 3 (B G R), 4 (B G R A), where 0 means auto
 * - Execpted QImage format is QImage::Format_Indexed8, Format_RGB32, Format_RGB888, Format_ARGB32
 * - Depth of generated cv::Mat is CV_8U
 */
cv::Mat image2Mat(const QImage &img, int mat_channels)
{
    Q_ASSERT(mat_channels == 0 || mat_channels == 1 || mat_channels == 3 || mat_channels == 4);

    if (mat_channels != 0 && mat_channels != 1 && mat_channels != 3 && mat_channels != 4)
        return cv::Mat();

    if (img.isNull())
        return cv::Mat();

    QImage image;
    switch (img.format()) {
    case QImage::Format_Indexed8:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_RGB888:
        image = img;
        break;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        image = img.convertToFormat(QImage::Format_Indexed8);
        break;
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB666:
    case QImage::Format_RGB16:
        image = img.convertToFormat(QImage::Format_RGB888);
        break;
    default:
        image = img.convertToFormat(QImage::Format_RGB32);
        break;
    }

    int channels = mat_channels == 0 ? image.depth()/8 : mat_channels;
    cv::Mat mat = cv::Mat(image.height(), image.width(), CV_8UC(channels));

    if (channels == 1) {
        for (int i=0; i<mat.rows; ++i) {
            const uchar * data = image.scanLine(i);
            if (image.format() == QImage::Format_Indexed8) {
                std::memcpy(mat.row(i).data, data, img.width());
            } else {
                for (int j=0; j<mat.cols; ++j, data+=image.depth()/8)
                    mat.at<uchar>(i, j) = (*data * 3728 + *(data+1) * 19238 + *(data+2)*9798)/32768;
            }
        }
    } else if (channels == 3) {
        for (int i=0; i<mat.rows; ++i) {
            const uchar * data = image.scanLine(i);
            if (image.format() == QImage::Format_Indexed8) {
                for (int j=0; j<mat.cols; ++j, ++data) {
                    mat.at<cv::Vec3b>(i, j)[0] = *data;
                    mat.at<cv::Vec3b>(i, j)[1] = *data;
                    mat.at<cv::Vec3b>(i, j)[2] = *data;
                }
            } else if (image.format() == QImage::Format_RGB888) {
                std::memcpy(mat.row(i).data, data, img.width()*3);
            } else {
                for (int j=0; j<mat.cols; ++j, data+=4) {
                    mat.at<cv::Vec3b>(i, j)[0] = data[0];
                    mat.at<cv::Vec3b>(i, j)[1] = data[1];
                    mat.at<cv::Vec3b>(i, j)[2] = data[2];
                }
            }
        }
    } else if (channels == 4) {
        for (int i=0; i<mat.rows; ++i) {
            const uchar * data = image.scanLine(i);
            if (image.format() == QImage::Format_Indexed8) {
                for (int j=0; j<mat.cols; ++j, ++data) {
                    mat.at<cv::Vec4b>(i, j)[0] = *data;
                    mat.at<cv::Vec4b>(i, j)[1] = *data;
                    mat.at<cv::Vec4b>(i, j)[2] = *data;
                    mat.at<cv::Vec4b>(i, j)[3] = 255;
                }
            } else if (image.format() == QImage::Format_RGB888) {
                for (int j=0; j<mat.cols; ++j, data+=3) {
                    mat.at<cv::Vec4b>(i, j)[0] = data[0];
                    mat.at<cv::Vec4b>(i, j)[1] = data[1];
                    mat.at<cv::Vec4b>(i, j)[2] = data[2];
                    mat.at<cv::Vec4b>(i, j)[3] = 255;
                }
            } else {
                std::memcpy(mat.row(i).data, data, img.width()*4);
            }
        }
    }

    return mat;
}

/* Convert cv::Mat to QImage
 *
 * Type of mat should be CV_8UC(n), CV_16UC(n), CV_32FC(n), where n is 1, 3, 4
 *
 * Format of QImage should be RGB32 RGB888 or Indexed8,
 * color channels of cv::Mat should stored in B G R order.
 */
QImage mat2Image(const cv::Mat & mat, QImage::Format format)
{
    Q_ASSERT(mat.depth()==CV_8U || mat.depth()==CV_16U ||mat.depth()==CV_32F);
    Q_ASSERT(format == QImage::Format_RGB32 || format == QImage::Format_RGB888 || format == QImage::Format_Indexed8);

    if (mat.empty())
        return QImage();

    cv::Mat mat_b;

    if (mat.depth() == CV_8U)
        mat_b = mat;
    else if (mat.depth() == CV_16U)
        mat.convertTo(mat_b, CV_8U, 255./65535.);
    else if (mat.depth() == CV_32F)
        mat.convertTo(mat_b, CV_8U, 255.);

    QImage outImage;

    if (format == QImage::Format_RGB32) {
        outImage = QImage(mat_b.cols, mat.rows, QImage::Format_RGB32);
        for (int i=0; i<mat_b.rows; ++i) {
            uchar * data = outImage.scanLine(i);
            if (mat_b.type() == CV_8UC1) {

                for (int j=0; j<mat_b.cols; ++j) {
                    *data++ = mat_b.at<uchar>(i, j);
                    *data++ = mat_b.at<uchar>(i, j);
                    *data++ = mat_b.at<uchar>(i, j);
                    *data++ = 255;
                }
            } else if (mat_b.type() == CV_8UC3) {
                for (int j=0; j<mat_b.cols; ++j) {
                    *data++ = mat_b.at<cv::Vec3b>(i, j)[0];
                    *data++ = mat_b.at<cv::Vec3b>(i, j)[1];
                    *data++ = mat_b.at<cv::Vec3b>(i, j)[2];
                    *data++ = 255;
                }
            } else if (mat_b.type() == CV_8UC4) {
                std::memcpy(data, mat_b.row(i).data, mat_b.cols*4);
            }
        }
    } else if (format == QImage::Format_RGB888){
        outImage = QImage(mat_b.cols, mat.rows, QImage::Format_RGB888);
        for (int i=0; i<mat_b.rows; ++i) {
            uchar * data = outImage.scanLine(i);
            if (mat_b.type() == CV_8UC1) {
                for (int j=0; j<mat_b.cols; ++j) {
                    *data++ = mat_b.at<uchar>(i, j);
                    *data++ = mat_b.at<uchar>(i, j);
                    *data++ = mat_b.at<uchar>(i, j);
                }
            } else if (mat_b.type() == CV_8UC3) {
                std::memcpy(data, mat_b.row(i).data, mat_b.cols*3);
            } else if (mat_b.type() == CV_8UC4) {
                for (int j=0; j<mat_b.cols; ++j) {
                    *data++ = mat_b.at<cv::Vec4b>(i, j)[0];
                    *data++ = mat_b.at<cv::Vec4b>(i, j)[1];
                    *data++ = mat_b.at<cv::Vec4b>(i, j)[2];
                }
            }
        }
    } else if (format == QImage::Format_Indexed8) {
        if (mat_b.type() == CV_8UC3)
            cv::cvtColor(mat_b, mat_b, CV_BGR2GRAY, 1);
        else if (mat_b.type() == CV_8UC4)
            cv::cvtColor(mat_b, mat_b, CV_BGRA2GRAY, 1);

        outImage = QImage(mat_b.cols, mat_b.rows, QImage::Format_Indexed8);

        QVector<QRgb> colorTable;
        for (int i=0; i<256; ++i)
            colorTable.append(qRgb(i,i,i));
        outImage.setColorTable(colorTable);

        for (int i=0; i<mat_b.rows; ++i) {
            uchar * data = outImage.scanLine(i);
            std::memcpy(data, mat_b.row(i).data, mat_b.cols);
        }
    }

    return outImage;
}

/* Convert QImage to cv::Mat without data copy
 *
 * - Supported QImage format is QImage::Format_Indexed8, Format_RGB888, Format_RGB32, Format_ARGB32
 * - Type of generated cv::Mat is CV_8UC1, CV_8UC3 (B G R), CV_8UC4 (B G R A),
 */
cv::Mat image2Mat_shared(const QImage &img)
{
    Q_ASSERT(img.format() == QImage::Format_Indexed8 || img.format() == QImage::Format_RGB888
             || img.format() == QImage::Format_RGB32 || img.format() == QImage::Format_ARGB32);

    if (img.isNull())
        return cv::Mat();

    switch (img.format()) {
    case QImage::Format_Indexed8:
    case QImage::Format_RGB888:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        return cv::Mat(img.height(), img.width(), CV_8UC(img.depth()/8), (uchar*)img.bits(), img.bytesPerLine());
    default:
        return cv::Mat();
    }
}

/* Convert  cv::Mat to QImage without data copy
 *
 * - Supported type of cv::Mat is CV_8UC1, CV_8UC3 (B G R), CV_8UC4 (B G R A)
 * - QImage format is QImage::Format_Indexed8, Format_RGB888, Format_ARGB32
 */
QImage mat2Image_shared(const cv::Mat &mat)
{
    Q_ASSERT(mat.type() == CV_8UC1 || mat.type() == CV_8UC3 || mat.type() == CV_8UC4);

    if (mat.empty())
        return QImage();

    QImage img;
    if (mat.type() == CV_8UC1) {
        img = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8);
        QVector<QRgb> colorTable;
        for (int i=0; i<256; ++i)
            colorTable.append(qRgb(i,i,i));
        img.setColorTable(colorTable);
    } else if (mat.type() == CV_8UC3) {
        img = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    } else if (mat.type() == CV_8UC4) {
        img = QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
    }

    return img;
}

} //namespace QtOcv