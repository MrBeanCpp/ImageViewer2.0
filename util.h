#ifndef UTIL_H
#define UTIL_H

#include <QWidget>
#include <Windows.h>

namespace Util {
    bool setWindowTop(QWidget* widget, bool top);
    QString getDirPath(const QString& filePath);
    QString getFileName(const QString& filePath);
    QString getWindowText(HWND hwnd);
    QString getProcessExePath(HWND hwnd);
    QString getFileDescription(const QString& path);
    QPoint getWindowPos(HWND hwnd);
}

#endif // UTIL_H
