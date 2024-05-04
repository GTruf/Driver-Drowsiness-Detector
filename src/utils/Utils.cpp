// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#include "Utils.h"
#include <opencv2/imgproc.hpp>
#include <QImage>

using namespace cv;

// https://asmaloney.com/2013/11/code/converting-between-cvmat-and-qimage-or-qpixmap/

Mat ToCvMat(const QImage& image) {
    switch (image.format()) {
        // 8-bit, 4 channel
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
            return {image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine())};

        // 8-bit, 3 channel
        case QImage::Format_RGB32:
        case QImage::Format_RGB888:
        {
            QImage swapped = image;

            if (image.format() == QImage::Format_RGB32 )
                swapped = swapped.convertToFormat(QImage::Format_RGB888);

            swapped = swapped.rgbSwapped();
            return cv::Mat(swapped.height(), swapped.width(), CV_8UC3, swapped.bits(), static_cast<size_t>(swapped.bytesPerLine())).clone();
        }

        // 8-bit, 1 channel
        case QImage::Format_Indexed8:
            return {image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine())};
        default:
            return {};
    }
}

QImage ToQImage(const Mat& mat) {
    switch (mat.type()) {
        case CV_8UC4: // 8-bit, 4 channel
            return {mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_ARGB32};
        case CV_8UC3: // 8-bit, 3 channel
            return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888).rgbSwapped();
        case CV_8UC1: // 8-bit, 1 channel
            return {mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8};
        default:
            return {};
    }
}
