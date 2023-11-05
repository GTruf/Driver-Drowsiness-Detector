// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2023                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#include "Camera.h"

Camera::Camera(int preferredResolutionWidth, int preferredResolutionHeight)
    : _preferredResolution(preferredResolutionWidth, preferredResolutionHeight)
{
    SetCameraDevice(0);
}

// Get the actual camera resolution
cv::Size Camera::GetResolution(const cv::Size& thresholdResolution) const {
    auto actualResolution = cv::Size(static_cast<int>(_videoCapture.get(cv::CAP_PROP_FRAME_WIDTH)),
                                     static_cast<int>(_videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT)));

    if (std::tie(actualResolution.width, actualResolution.height) > std::tie(thresholdResolution.width, thresholdResolution.height))
        return thresholdResolution;

    return actualResolution;
}

bool Camera::IsAvailable() const {
    return _isAvailable;
}

void Camera::SetCameraDevice(int cameraDeviceIndex) {
    // Very fine-tuning, perhaps not all systems will be suitable for this combination (with MJPEG) to get
    // high FPS when capturing frames from the camera
    _isAvailable = _videoCapture.open(cameraDeviceIndex, cv::CAP_DSHOW);

    // If possible, this resolution will be used
    _videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, _preferredResolution.width);
    _videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, _preferredResolution.height);
    _videoCapture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
}
