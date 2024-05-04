// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#ifndef TEXTBROWSERLOGGER_H
#define TEXTBROWSERLOGGER_H

#include <QString>

class QTextBrowser;
class QColor;

class TextBrowserLogger {
public:
    static void Clear(QTextBrowser* textBrowser);
    static void Log(QTextBrowser* textBrowser, const QString& message, const QColor& color,
                    const QString& dateFormat = "dd.MM.yyyy hh:mm:ss.zzz");
};

#endif // TEXTBROWSERLOGGER_H
