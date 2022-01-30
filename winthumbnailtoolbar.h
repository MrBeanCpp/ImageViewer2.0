#ifndef WINTHUMBNAILTOOLBAR_H
#define WINTHUMBNAILTOOLBAR_H

#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00

#include "windows.h" //保证windows.h中的版本定义被修改 放第一个
#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QPixmap>
#include <QtWinExtras>
#include <dwmapi.h>
class WinThumbnailToolBar : public QWinThumbnailToolBar, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    explicit WinThumbnailToolBar(QObject *parent = nullptr);
    void updateThumbnailPixmap();
    void setThumbnailPixmap(const QPixmap& pixmap, const QSize& maxSize);

signals:
    void thumbnailRequested(QSize maxSize);

    // QAbstractNativeEventFilter interface
public:
    bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

private:
    //QPixmap thumbnail;
};

#endif // WINTHUMBNAILTOOLBAR_H
