#include "winthumbnailtoolbar.h"
#include <QDebug>
WinThumbnailToolBar::WinThumbnailToolBar(QObject* parent)
    : QWinThumbnailToolBar(parent)
{
    qApp->installNativeEventFilter(this); //后安装filter的先获取nativeEvent 所以在↑父类构造之后
}

void WinThumbnailToolBar::updateThumbnailPixmap()
{
    if (window() == nullptr) return;
    DwmInvalidateIconicBitmaps((HWND)window()->winId()); //通知DWM缩略图失效，下次需要缩略图时(鼠标移至任务栏图标，而非立即)会重新获取
}

void WinThumbnailToolBar::setThumbnailPixmap(const QPixmap& pixmap, const QSize& maxSize)
{
    if (window() == nullptr || pixmap.isNull()) return;
    //thumbnail = pixmap;

    HBITMAP hbm = QtWin::toHBITMAP(pixmap.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    if (hbm) {
        HRESULT hr = DwmSetIconicThumbnail((HWND)window()->winId(), hbm, 0); //0不显示frame Qt默认DWM_SIT_DISPLAYFRAME 显示框架
        DeleteObject(hbm);
        qDebug() << ((hr == S_OK) ? "#class# SetIconicThumbnail SUCCESS" : "#class# SetIconicThumbnail Failed, Retrying");
    } else
        qDebug() << "hbm error";
}

bool WinThumbnailToolBar::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    const MSG* msg = static_cast<const MSG*>(message);

    switch (msg->message) {
    case WM_DWMSENDICONICTHUMBNAIL: {
        const QSize maxSize(HIWORD(msg->lParam), LOWORD(msg->lParam));
        //qDebug() << "#class# WM_DWMSENDICONICTHUMBNAIL" << maxSize;
        //setThumbnailPixmap(thumbnail, maxSize);
        emit thumbnailRequested(maxSize);
        return true;
    } break;
    }
    return false;
}
