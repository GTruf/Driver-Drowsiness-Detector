// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2023                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#ifndef IMAGERECOGNIZER_H
#define IMAGERECOGNIZER_H

#include <opencv2/dnn.hpp>

// Aka "class type"
enum RecognitionType {
    AttentiveEye,
    DrowsyEye,
    FistGesture,
    PalmGesture,
    VGesture,
    RecognitionTypeCount
};

struct RecognitionInfo {
    RecognitionType recognitionType;
    cv::Rect boundingBox;
    float confidence;

    explicit RecognitionInfo(RecognitionType type, const cv::Rect& box, float objectConfidence) :
        recognitionType(type), boundingBox(box), confidence(objectConfidence) {}
};

class ImageRecognizer {
public:
    explicit ImageRecognizer(std::string modelPath, const cv::Size& blobSize = cv::Size(640, 640));
    const std::string& Error() const;
    std::vector<RecognitionInfo> Recognize(const cv::Mat& inputImage, float confidenceThreshold = 0.75, float classScoreThreshold = 0.8);
    void Restart();

private:
    std::unique_ptr<cv::dnn::Net> _networkModel;
    cv::Size _blobSize;
    std::string _modelPath, _error;

    std::vector<cv::Mat> ForwardPropagate(const cv::Mat& inputImage);
    std::vector<RecognitionInfo> AnalyseDetections(const cv::Mat& inputImage, std::vector<cv::Mat>&& detections,
                                                   float confidenceThreshold, float classScoreThreshold) const;
};

#endif // IMAGERECOGNIZER_H
