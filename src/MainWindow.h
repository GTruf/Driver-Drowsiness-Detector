// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Camera.h"
#include "RecognitionFrameCounter.h"
#include <QMainWindow>
#include <QTranslator>
#include <QSoundEffect>
#include <QUrl>
#include <QMediaDevices>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    bool Errors() const;

signals:
    void LogAsync(const QString& message, const QColor& color); // Signal for the main thread to insert text into QTextBrowser
    void RecognitionSystemRestarted();
    void PlayWarningSignal();
    void UpdateVideoFrame(const QPixmap& videoFrame);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void OnAvailableCamerasActivated(int index);
    void OnRunStopClicked();
    void OnClearLoggerClicked();
    void OnVideoInputsChanged();
    void OnRecognitionSystemRestarted();

private:
    Ui::MainWindow* _ui;
    bool _hasErrors = false;
    QTranslator _uiRussianTranslator;

    QSoundEffect _warningSound;

    bool _isCameraRun = false;
    Camera _camera;
    QMediaDevices _mediaDevices;
    QString _currentCameraName;
    ImageRecognizer _imageRecognizer;

    bool _isVideoStreamThreadRun = false;
    QMutex _mutex;
    QThread* _videoStreamThread = nullptr;
    QWaitCondition _videoStreamStopper;

    // Sleep mode for the drowsiness recognition system
    bool _isSleepMode = false; // The V gesture sets true, the Palm gesture sets false

    // The idea is that if the number of frames with object recognition reaches a certain number, then it will be
    // concluded that this object is actually recognized by the neural network for some time. Lowering UPPER_BOUND
    // results in faster recognition (15 is optimal for recognition within about one second at webcam speed of 30 fps
    // on a computer with the power to handle about that number of frames per second with the neural network)
    RecognitionFrameCounter<15> _frameCounterWithRecognition; // Counter of frames where there is recognition

    // The idea is that if the number of frames without recognition reaches a certain number, then the frame counter
    // with recognition will reset, which means that the object is really not recognized, and it is not an error of the
    // neural network, etc.
    RecognitionFrameCounter<5> _frameCounterWithoutRecognition; // Counter of frames where there is no recognition

    void Init();
    void InitConnects();
    void SetSystemInformation();

    void StartCamera();
    void StopCamera();

    ///// Executed not in the main thread
    void VideoStream();

    void HandleRecognitions(const std::vector<RecognitionInfo>& recognitions, cv::Mat& videoFrame);
    void RestartRecognitionSystem();
    void WakeUpDrowsinessRecognitionSystem();
    void PutDrowsinessRecognitionSystemIntoSleepMode();
    void DrowsinessAlert();
    //////////

    void ResumeVideoStreamThread();
    void SuspendVideoStreamThread(uint16_t msDelay);
    void DestroyVideoStreamThread();
};

#endif // MAINWINDOW_H
