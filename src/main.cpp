// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#include "MainWindow.h"
#include <QApplication>

int main(int argc, char** argv) {
#ifdef Q_OS_WIN64
    // https://github.com/opencv/opencv/issues/17687#issuecomment-872291073
    _putenv_s("OPENCV_VIDEOIO_MSMF_ENABLE_HW_TRANSFORMS", "0"); // So there is no delay when opening a camera with MSMF backend
#endif
    // TODO: Readme
    QApplication application(argc, argv);
    QTranslator translator;
    MainWindow mainWindow;

    if (mainWindow.Errors())
        return 1;

    mainWindow.showMaximized();
    return QApplication::exec();
}
