// +-----------------------------------------+
// |              License: MIT               |
// +-----------------------------------------+
// | Copyright (c) 2024                      |
// | Author: Gleb Trufanov (aka Glebchansky) |
// +-----------------------------------------+

#include "TextBrowserLogger.h"
#include <QTextBrowser>
#include <QDateTime>

void TextBrowserLogger::Clear(QTextBrowser* textBrowser) {
    if (!textBrowser)
        throw std::invalid_argument("The pointer to QTextBrowser should not be nullptr");

    textBrowser->clear();
}

void TextBrowserLogger::Log(QTextBrowser* textBrowser, const QString& message, const QColor& color, const QString& dateFormat) {
    if (!textBrowser)
        throw std::invalid_argument("The pointer to QTextBrowser should not be nullptr");

    auto textCursor = textBrowser->textCursor();
    textCursor.movePosition(QTextCursor::End);

    textBrowser->setTextCursor(textCursor);
    textBrowser->setTextColor(color);
    textBrowser->insertPlainText(QDateTime::currentDateTime().toString(dateFormat).append(": ").append(message).append('\n'));
}
