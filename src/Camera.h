// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2023                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#ifndef CAMERA_H
#define CAMERA_H

#include <opencv2/videoio.hpp>

class Camera {
public:
    explicit Camera(int preferredResolutionWidth = 3840, int preferredResolutionHeight = 2160);
    cv::Size GetResolution(const cv::Size& thresholdResolution = cv::Size(1280, 720)) const;
    bool IsAvailable() const;
    void SetCameraDevice(int cameraDeviceIndex);
    template <uint16_t READ_NUMBER> void Read(cv::Mat& frame);

private:
    cv::VideoCapture _videoCapture;
    cv::Size _preferredResolution;
    bool _isAvailable = false;
};

// The idea is that cv::VideoCapture::read() is executed in READ_NUMBER instances, effectively READ_NUMBER frames,
// skipping the first READ_NUMBER - 1 frames and recording only the last frame. This is a mini hack that speeds up
// webcam speed in OpenCV
// Taken from here: https://stackoverflow.com/a/71973860/17137664
template <uint16_t READ_NUMBER>
void Camera::Read(cv::Mat& frame) {
    for (uint16_t i = 0; i < READ_NUMBER; i++)
        _isAvailable = _videoCapture.read(frame);
}

#endif // CAMERA_H
