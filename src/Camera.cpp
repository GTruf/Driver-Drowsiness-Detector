// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
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
#ifdef _WIN64 // The Camera class is Qt independent, so no Q_OS_* type macros are used here
    _isAvailable = _videoCapture.open(cameraDeviceIndex, cv::CAP_DSHOW);

    if (!_isAvailable)
        _isAvailable = _videoCapture.open(cameraDeviceIndex, cv::CAP_MSMF);
#else
    _isAvailable = _videoCapture.open(cameraDeviceIndex, cv::CAP_V4L2);
#endif

    if (_isAvailable) {
        // If possible, this resolution will be used
        _videoCapture.set(cv::CAP_PROP_FRAME_WIDTH, _preferredResolution.width);
        _videoCapture.set(cv::CAP_PROP_FRAME_HEIGHT, _preferredResolution.height);

        auto const cameraBackendName = _videoCapture.getBackendName();

        // Very fine-tuning, perhaps not all systems will be suitable for this combination (DSHOW/V4L2 with MJPEG) to get
        // high FPS when capturing frames from the camera
        if (cameraBackendName == "DSHOW" || cameraBackendName == "V4L2")
            _videoCapture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    }
}
