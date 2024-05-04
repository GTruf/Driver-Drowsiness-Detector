// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#include "ImageRecognizer.h"
#include <filesystem>

using namespace std;
using namespace cv;
using namespace cv::dnn;

ImageRecognizer::ImageRecognizer(string modelPath, const Size& blobSize) : _blobSize(blobSize),
    _modelPath(std::move(modelPath))
{
    if (!filesystem::exists(_modelPath)) {
        _error = std::string("No neural network model file found at \"").append(_modelPath).append("\"");
        return;
    }

    try {
        Restart();
    }
    catch (const cv::Exception& ex) {
        _error = ex.what();
    }

    // See the discussion here about FP16 and FP32
    // https://forums.developer.nvidia.com/t/fp16-support-on-gtx-1060-and-1080/53256
    //_networkModel.setPreferableTarget(CV_FP16_TYPE ? DNN_TARGET_CUDA_FP16 : DNN_TARGET_CUDA);
}

const std::string& ImageRecognizer::Error() const {
    return _error;
}

vector<RecognitionInfo> ImageRecognizer::Recognize(const Mat& inputImage, float confidenceThreshold, float classScoreThreshold) {
    auto detections = ForwardPropagate(inputImage);
    return AnalyseDetections(inputImage, std::move(detections), confidenceThreshold, classScoreThreshold);
}

void ImageRecognizer::Restart() {
    _networkModel = make_unique<cv::dnn::Net>();
    *_networkModel = readNetFromONNX(_modelPath);
    _networkModel->setPreferableBackend(DNN_BACKEND_CUDA);
    _networkModel->setPreferableTarget(DNN_TARGET_CUDA);
}

vector<Mat> ImageRecognizer::ForwardPropagate(const Mat& inputImage) {
    static constexpr double BLOB_SCALE_FACTOR = 1. / 255.;

    vector<Mat> detections;
    Mat blob;

    blobFromImage(inputImage, blob, BLOB_SCALE_FACTOR, _blobSize, Scalar(), true);
    _networkModel->setInput(blob);
    _networkModel->forward(detections, _networkModel->getUnconnectedOutLayersNames());

    return detections;
}

vector<RecognitionInfo> ImageRecognizer::AnalyseDetections(const Mat& inputImage, vector<Mat>&& detections,
                                                           float confidenceThreshold, float classScoreThreshold) const
{
    // Check the structure of the object returned by the "Net::forward" function here
    // https://learnopencv.com/object-detection-using-yolov5-and-opencv-dnn-in-c-and-python/

    // 25200 rows (detections or regression bounding boxes) with the structure below (in this case 25200x10)
    // ╔═══╤═══╤═══╤═══╤════════════╤═══════════════════════════╗
    // ║ X │ Y │ W │ H │ Confidence │ Class scores of N classes ║
    // ╚═══╧═══╧═══╧═══╧════════════╧═══════════════════════════╝

    static constexpr uint16_t ONNX_ROWS = 25200; // 25200 for size 640x640
    static constexpr uint16_t DETECTION_INFORMATION_SIZE = 10;
    static constexpr float NMS_THRESHOLD = 0.45;

    vector<RecognitionInfo> recognitions; // Output result
    vector<int> classIds;
    vector<float> confidences;
    vector<Rect> boundingBoxes;

    // Resizing factor
    const auto xFactor = static_cast<float>(inputImage.cols) / static_cast<float>(_blobSize.width);
    const auto yFactor = static_cast<float>(inputImage.rows) / static_cast<float>(_blobSize.height);

    auto* data = reinterpret_cast<float*>(detections[0].data);

    for (size_t row = 0; row < ONNX_ROWS; row++) {
        float confidence = data[4];

        if (confidence >= confidenceThreshold) { // Discard bad detections and continue
            float* classesScores = data + 5;
            Point classId;
            double maxClassScore;

            // Create a 1xRecognitionTypeCount Mat and store class scores of RecognitionTypeCount classes
            Mat scores(1, RecognitionTypeCount, CV_32FC1, classesScores);

            // Perform minMaxLoc and acquire the index of best class score
            minMaxLoc(scores, nullptr, &maxClassScore, nullptr, &classId);

            if (maxClassScore > classScoreThreshold) {
                confidences.push_back(confidence);
                classIds.push_back(classId.x);

                // Center
                float cx = data[0];
                float cy = data[1];

                // Box dimension
                float w = data[2];
                float h = data[3];

                // Bounding box coordinates
                int left = static_cast<int>((cx - 0.5 * w) * xFactor);
                int top = static_cast<int>((cy - 0.5 * h) * yFactor);
                int width = static_cast<int>(w * xFactor);
                int height = static_cast<int>(h * yFactor);

                // Store good detections in the boxes vector
                boundingBoxes.emplace_back(left, top, width, height);
            }
        }

        data += DETECTION_INFORMATION_SIZE; // Jump to the next row
    }

    // Perform Non-Maximum Suppression
    vector<int> indices;
    NMSBoxes(boundingBoxes, confidences, classScoreThreshold, NMS_THRESHOLD, indices);

    for (int index : indices)
        recognitions.emplace_back(RecognitionType(classIds[index]), boundingBoxes[index], confidences[index]);

    return recognitions;
}
