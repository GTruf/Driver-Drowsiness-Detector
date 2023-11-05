// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2023                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "Utils.h"
#include "TextBrowserLogger.h"
#include <opencv2/imgproc.hpp>
#include <QDir>
#include <QSysInfo>
#include <QSoundEffect>
#include <QProcess>
#include <QMessageBox>
#include <QCameraDevice>
#include <QTimer>
#include <set>

using namespace std;
using namespace cv;

namespace {

constexpr int CAMERA_CONTENT_HINT_LABEL_PAGE  = 0;
constexpr int VIDEO_FRAME_PAGE = 1;
constexpr int RESTART_RECOGNITION_SYSTEM_PAGE = 2;

constexpr uint16_t FRAME_READ_NUMBER = 1; // https://stackoverflow.com/a/71973860/17137664
constexpr uint16_t DROWSINESS_SLEEP_TIME_MS = 3000;
constexpr uint16_t RESTART_SLEEP_TIME_MS = 1000;

constexpr QColor PALE_YELLOW = {255, 219, 139};
constexpr QColor CRAIOLA_PERIWINKLE = {197, 208, 230};
constexpr QColor DEEP_CARMINE_PINK = {239, 48, 56};
constexpr QColor WISTERIA = {201, 160, 20};

string ToClassName(RecognitionType recognitionType) {
    switch (recognitionType) {
        case AttentiveEye: return QObject::tr("Attentive Eye").toStdString();
        case DrowsyEye:    return QObject::tr("Drowsy Eye").toStdString();
        case FistGesture:  return QObject::tr("Fist Gesture").toStdString();
        case PalmGesture:  return QObject::tr("Palm Gesture").toStdString();
        case VGesture:     return QObject::tr("V Gesture").toStdString();
        default:           return "";
    }
}

Scalar ToRecognitionColor(RecognitionType recognitionType) {
    // BGR format
    switch (recognitionType) {
        case AttentiveEye: return {69, 89, 30};
        case DrowsyEye:    return {137, 49, 66};
        case FistGesture:  return {88, 93, 188};
        case PalmGesture:  return {0, 110, 215};
        case VGesture:     return {167, 123, 0};
        default:           return {0, 0, 0};
    }
}

void DrawLabel(Mat& image, const string& text, int left, int top, const Scalar& lineRectangleColor) {
    int baseLine;
    Size labelSize = getTextSize(text, FONT_HERSHEY_COMPLEX, 0.5, 1, &baseLine);
    top = max(top, labelSize.height);

    Point cornerLineTopRight(left + 15, top - 20);

    // For some reason getTextSize does not take "%" into account
    const Point horizontalLineBottomRight(left + labelSize.width + 21, top - 20);
    const Point rectangleTopRight(horizontalLineBottomRight.x, horizontalLineBottomRight.y - labelSize.height - baseLine);

    line(image, Point(left, top), cornerLineTopRight, lineRectangleColor, 2, LINE_AA);
    line(image, cornerLineTopRight, horizontalLineBottomRight, lineRectangleColor, 2, LINE_AA);
    rectangle(image, cornerLineTopRight, rectangleTopRight, lineRectangleColor, FILLED, LINE_AA);

    cornerLineTopRight.x += 3;
    cornerLineTopRight.y -= labelSize.height * 0.25;
    putText(image, text, cornerLineTopRight, FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255), 1, LINE_AA); // BGR format
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow),
    _imageRecognizer(QDir::currentPath().append("/resource/neural_network_model/neural_network_model.onnx").toStdString())
{
    auto warningSoundPath = QDir::currentPath().append("/resource/sound/warning_sound.wav");

    if (!QFileInfo::exists(warningSoundPath)) {
        QMessageBox::critical(nullptr, "Error when initialising the warning sound",
                              QString("No warning sound file found at \"").append(warningSoundPath).append("\""));
        _hasErrors = true;
        return;
    }

    if (!_imageRecognizer.Error().empty()) {
        QMessageBox::critical(nullptr, tr("Error when initialising the recogniser"), tr(_imageRecognizer.Error().c_str()));
        _hasErrors = true;
        return;
    }

    _warningSound.setSource(QUrl::fromLocalFile(warningSoundPath));

    ui->setupUi(this);
    Init();
}

bool MainWindow::Errors() const {
    return _hasErrors;
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);

        // Explicitly setting text is needed because calling "ui->retranslateUi(this)" resets the text to the one
        // originally set for the button in the GUI (Qt Designer). This is because retranslate only calls
        // QCoreApplication::translate for the GUI caption (in this case the "Run" caption)
        if (_isCameraRun) ui->runStop->setText(tr("Stop"));
    }

    QWidget::changeEvent(event);
}

void MainWindow::OnAvailableCamerasActivated(int index) {
    auto selectedCameraName = ui->availableCameras->itemText(index);

    if (selectedCameraName == _currentCameraName) // To avoid re-setting the same camera device
        return;

    bool isNeedToResumeVideoStream = false;

    if (_isCameraRun) {
        SuspendVideoStreamThread(175); // 175 ms should be enough for the video stream thread to fall asleep

        // If changed the camera while the previous one was working, then later after changing the camera it will be
        // necessary to resume the video stream thread
        isNeedToResumeVideoStream = true;
    }

    ui->videoFrame->clear();
    _currentCameraName = std::move(selectedCameraName);
    _camera.SetCameraDevice(index);

    const auto newCameraResolution = _camera.GetResolution();
    ui->cameraContentStackedWidget->setFixedSize(newCameraResolution.width, newCameraResolution.height);

    TextBrowserLogger::Log(ui->logger, tr("The camera \"").append(_currentCameraName).append(tr("\" is selected")), CRAIOLA_PERIWINKLE);

    if (isNeedToResumeVideoStream)
        ResumeVideoStreamThread();
}

void MainWindow::OnRunStopClicked() {
    if (!_isCameraRun && !_camera.IsAvailable()) {
        QMessageBox::warning(nullptr, tr("No available cameras"), tr("No available cameras"));
        return;
    }

    _isCameraRun = !_isCameraRun;
    ui->runStop->setText(_isCameraRun ? tr("Stop") : tr("Run"));

    _isCameraRun ? StartCamera() : StopCamera();
}

void MainWindow::OnClearLoggerClicked() {
    TextBrowserLogger::Clear(ui->logger);
}

void MainWindow::OnVideoInputsChanged() {
    const QList<QCameraDevice> cameras = _mediaDevices.videoInputs();
    QList<QString> existingCameras; // There may be a situation where there are cameras with the same names (descriptions)

    ui->availableCameras->clear();

    for (const auto& camera : cameras) {
        const auto camerasCount = existingCameras.count(camera.description());
        const QString cameraName = camera.description().append(camerasCount > 0 ? QString::number(camerasCount) : "");

        ui->availableCameras->addItem(cameraName);
        existingCameras.emplace_back(camera.description());
    }

    const auto isCurrentCameraDeviceAvailable = existingCameras.contains(_currentCameraName);

    if (_isCameraRun && !isCurrentCameraDeviceAvailable) {
        const auto errorMessage = tr("There is a problem with the camera ").append(_currentCameraName);

        OnRunStopClicked();
        TextBrowserLogger::Log(ui->logger, errorMessage, DEEP_CARMINE_PINK);
        QMessageBox::warning(nullptr, tr("Camera failure"), errorMessage);
    }

    if (cameras.isEmpty()) {
        _camera.SetCameraDevice(0); // Camera will be unavailable after VideoCapture::open
    }
    else if (!_camera.IsAvailable() || !isCurrentCameraDeviceAvailable) {
        _camera.SetCameraDevice(0); // If there were no cameras available before or the current camera device has been deactivated
        const auto cameraResolution = _camera.GetResolution();
        ui->cameraContentStackedWidget->setFixedSize(cameraResolution.width, cameraResolution.height);
        _currentCameraName = existingCameras[0];
        ui->availableCameras->setCurrentIndex(0);
    }
    else {
        ui->availableCameras->setCurrentText(_currentCameraName);
    }
}

void MainWindow::OnRecognitionSystemRestarted() {
    ui->cameraContentStackedWidget->setCurrentIndex(RESTART_RECOGNITION_SYSTEM_PAGE);
    ui->cameraContentStackedWidget->repaint();

    _frameCounterWithRecognition.Reset(FistGesture);
    DestroyVideoStreamThread();
    _imageRecognizer.Restart();

    QThread::msleep(RESTART_SLEEP_TIME_MS); // A pause for a sense of duration in restarting the recognition system
    StartCamera();
}

void MainWindow::Init() {
    SetSystemInformation();

    ui->russianLanguage->setIcon(QIcon(":/icons/images/Flag_of_Russia.svg"));
    ui->englishLanguage->setIcon(QIcon(":/icons/images/Flag_of_the_United_Kingdom.svg"));

    OnVideoInputsChanged(); // For fill combobox on init
    InitConnects();

    if (ui->availableCameras->count())
        _currentCameraName = ui->availableCameras->itemText(0);

    showMaximized();

    const auto translationFilePath = QDir::currentPath().append("/resource/translation/ISS-Driver-Drowsiness-Detector_ru_RU.qm");
    const auto translationFilePathErrorMessage = QString("Error when loading the translation file \"%1\". "
        "The Russian translation will not be available during the work of the application.").arg(translationFilePath);

    if (!_uiRussianTranslator.load(translationFilePath)) {
        ui->russianLanguage->setDisabled(true);
        QMessageBox::warning(nullptr, "Error when loading the translation file", translationFilePathErrorMessage);
    }
}

void MainWindow::InitConnects() {
    // Connect for update available cameras if the list of cameras in the system has changed
    connect(&_mediaDevices, &QMediaDevices::videoInputsChanged, this, &MainWindow::OnVideoInputsChanged);

    for (const auto* retranslateUiButton : {ui->russianLanguage, ui->englishLanguage}) {
        connect(retranslateUiButton, &QAction::triggered, this, [&, retranslateUiButton]() {
            if (retranslateUiButton == ui->russianLanguage) {
                QApplication::installTranslator(&_uiRussianTranslator);
            }
            else {
                QApplication::removeTranslator(&_uiRussianTranslator);
            }
        });
    }

    connect(ui->availableCameras, &QComboBox::activated, this, &MainWindow::OnAvailableCamerasActivated);
    connect(ui->runStop, &QPushButton::clicked, this, &MainWindow::OnRunStopClicked);
    connect(ui->clearLogger, &QPushButton::clicked, this, &MainWindow::OnClearLoggerClicked);
    connect(this, &MainWindow::RecognitionSystemRestarted, this, &MainWindow::OnRecognitionSystemRestarted);
    connect(this, &MainWindow::PlayWarningSignal, this, [this]() { _warningSound.play(); });
}

void MainWindow::SetSystemInformation() {
    QProcess systemProcess;
    QString cpuName, gpuName;

#ifdef Q_OS_WIN64
    systemProcess.startCommand("wmic cpu get name");
    systemProcess.waitForFinished();
    cpuName = systemProcess.readAllStandardOutput();
    cpuName.remove(0, cpuName.indexOf('\n') + 1);
    cpuName = cpuName.trimmed();

    systemProcess.startCommand("wmic PATH Win32_videocontroller get VideoProcessor");
    systemProcess.waitForFinished();
    gpuName = systemProcess.readAllStandardOutput();
    gpuName.remove(0, gpuName.indexOf('\n') + 1);
    gpuName = gpuName.trimmed();
#endif
#ifdef Q_OS_LINUX
    systemProcess.start("\"cat /proc/cpuinfo | grep 'model name' | uniq\"");
    systemProcess.waitForFinished();
    cpuName = systemProcess.readAllStandardOutput();
    // TODO System Information in Linux
#endif

    ui->operatingSystem->setText(QSysInfo::prettyProductName());
    ui->cpuName->setText(cpuName);
    ui->gpuName->setText(gpuName);
}

void MainWindow::StartCamera() {
    if (!_videoStreamThread || !_videoStreamThread->isRunning()) {
        _videoStreamThread = QThread::create(&MainWindow::VideoStream, this);
        _isVideoStreamThreadRun = true;
        _videoStreamThread->start();
    }
    else {
        ResumeVideoStreamThread();
    }

    TextBrowserLogger::Log(ui->logger, tr("The camera is running"), PALE_YELLOW);
    ui->cameraContentStackedWidget->setCurrentIndex(VIDEO_FRAME_PAGE);
}

void MainWindow::StopCamera() {
    ui->videoFrame->clear();
    ui->cameraContentStackedWidget->setCurrentIndex(CAMERA_CONTENT_HINT_LABEL_PAGE);
    SuspendVideoStreamThread(50);
    TextBrowserLogger::Log(ui->logger, tr("The camera is stopped"), PALE_YELLOW);
}

void MainWindow::VideoStream() {
    while (_isVideoStreamThreadRun) {
         QMutexLocker locker(&_mutex);

        while (!_isCameraRun)
            _videoStreamStopper.wait(&_mutex);

        Mat videoFrame;
        _camera.Read<FRAME_READ_NUMBER>(videoFrame);

        if (!videoFrame.empty()) {
            Mat flippedVideoFrame;
            flip(videoFrame, flippedVideoFrame, 1);

            const auto recognitions = _imageRecognizer.Recognize(flippedVideoFrame);
            HandleRecognitions(recognitions, flippedVideoFrame);

            ui->videoFrame->setPixmap(QPixmap::fromImage(ToQImage(flippedVideoFrame)));
        }

        // When repeatedly stopping and starting the camera using the GUI without this explicit unlock call, the thread may stall
        // It seems that the QMutexLocker destructor is not always called in time (same thing with std::unique_lock)
        locker.unlock();
    }
}

void MainWindow::HandleRecognitions(const std::vector<RecognitionInfo>& recognitions, cv::Mat& videoFrame) {
    static char confidenceBuffer[10];
    int16_t drowsyEyeCount = 0;

    set<RecognitionType> unrecognizedClasses = { // Types of recognitions that are not in the frame
        AttentiveEye,
        DrowsyEye,
        FistGesture,
        PalmGesture,
        VGesture
    };

    for (const auto& recognitionInfo : recognitions) {
        if (ui->showDetections->isChecked()) {
            const auto recognitionColor = ToRecognitionColor(recognitionInfo.recognitionType);
            snprintf(confidenceBuffer, sizeof(confidenceBuffer), " %.2f %%", recognitionInfo.confidence * 100);

            rectangle(videoFrame, recognitionInfo.boundingBox, recognitionColor, 2); // Bounding Box
            DrawLabel(videoFrame, ToClassName(recognitionInfo.recognitionType).append(confidenceBuffer),
                      recognitionInfo.boundingBox.x, recognitionInfo.boundingBox.y, recognitionColor);
        }

        // There is no specific handling for the attentive eye
        if (!ui->debugMode->isChecked() && recognitionInfo.recognitionType != AttentiveEye) {
            if (recognitionInfo.recognitionType == DrowsyEye) {
                // The frame counter with drowsy eye recognition is increased only if 2 drowsy eyes are recognized
                if (!_isSleepMode && ++drowsyEyeCount == 2) {
                    if (_frameCounterWithRecognition.Increase(DrowsyEye)) { // If the frame counter with recognition is completely full
                        DrowsinessAlert();
                        return;
                    }

                    drowsyEyeCount = -1; // If two drowsy eyes are recognized, do not look for more drowsy eyes in the frame
                    _frameCounterWithoutRecognition.Reset(DrowsyEye);
                    unrecognizedClasses.erase(DrowsyEye);
                }
            }
            else {
                _frameCounterWithRecognition.Increase(recognitionInfo.recognitionType);
                _frameCounterWithoutRecognition.Reset(recognitionInfo.recognitionType);
                unrecognizedClasses.erase(recognitionInfo.recognitionType);
            }
        }
    }

    for (const auto unrecognizedClass : unrecognizedClasses) {
        if (_frameCounterWithoutRecognition.Increase(unrecognizedClass)) // If the frame counter without recognition is completely full
            _frameCounterWithRecognition.Reset(unrecognizedClass);
    }

    // Checks are in order of priority of recognitions
    if (_frameCounterWithRecognition.IsFull(FistGesture))
        RestartRecognitionSystem();
    else if (_frameCounterWithRecognition.IsFull(PalmGesture) && _isSleepMode)
        WakeUpDrowsinessRecognitionSystem();
    else if (_frameCounterWithRecognition.IsFull(VGesture) && !_isSleepMode)
        PutDrowsinessRecognitionSystemIntoSleepMode();
}

void MainWindow::RestartRecognitionSystem() {
    TextBrowserLogger::Log(ui->logger, tr("The Fist gesture is recognized"), WISTERIA);
    TextBrowserLogger::Log(ui->logger, tr("Restart the recognition system"), WISTERIA);

    emit RecognitionSystemRestarted();
}

void MainWindow::WakeUpDrowsinessRecognitionSystem() {
    TextBrowserLogger::Log(ui->logger, tr("The Palm gesture is recognized"), WISTERIA);
    TextBrowserLogger::Log(ui->logger, tr("Waking up the drowsiness recognition system"), WISTERIA);

    _isSleepMode = false;
    _frameCounterWithRecognition.Reset(PalmGesture);
}

void MainWindow::PutDrowsinessRecognitionSystemIntoSleepMode() {
    TextBrowserLogger::Log(ui->logger, tr("The V gesture is recognized"), WISTERIA);
    TextBrowserLogger::Log(ui->logger, tr("Putting the drowsiness recognition system into sleep mode"), WISTERIA);

    _isSleepMode = true;
    _frameCounterWithRecognition.Reset(VGesture);
}

void MainWindow::DrowsinessAlert() {
    TextBrowserLogger::Log(ui->logger, tr("Drowsiness is recognized"), WISTERIA);
    TextBrowserLogger::Log(ui->logger, tr("Drowsiness alert with a warning sound"), WISTERIA);

    emit PlayWarningSignal();

    _frameCounterWithRecognition.FullReset();
    _frameCounterWithoutRecognition.FullReset();
    _isSleepMode = true;

    // After drowsiness is detected, the drowsiness recognition system pauses for a few seconds
    QTimer::singleShot(DROWSINESS_SLEEP_TIME_MS, this, [this]() { _isSleepMode = false; });
}

void MainWindow::ResumeVideoStreamThread() {
    {
        const QMutexLocker locker(&_mutex);
        _isCameraRun = true;
    }

    _videoStreamStopper.notify_one();
}

void MainWindow::SuspendVideoStreamThread(uint16_t msDelay) {
    _isCameraRun = false;
    QThread::msleep(msDelay); // Delay so that the thread is exactly paused due to the condition _isCameraRun == false
}

void MainWindow::DestroyVideoStreamThread() {
    if (_videoStreamThread && _videoStreamThread->isRunning()) {
        _isVideoStreamThreadRun = false;
        ResumeVideoStreamThread(); // To unpause the thread if it is paused and immediately stop
        _videoStreamThread->wait();
        delete _videoStreamThread;
        _videoStreamThread = nullptr;
    }
}

MainWindow::~MainWindow() {
    DestroyVideoStreamThread();
    delete ui;
}
