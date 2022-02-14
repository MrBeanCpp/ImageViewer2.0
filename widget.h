#ifndef WIDGET_H
#define WIDGET_H

#include "winthumbnailtoolbar.h"
#include <QAbstractButton>
#include <QAbstractNativeEventFilter>
#include <QGraphicsEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPair>
#include <QRect>
#include <QTime>
#include <QWidget>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    bool isOnPixmap(const QPoint& curPos);
    int getPixels(const QSize& size);
    bool isInPixelRange(int pixels);
    bool isInPixelRange(const QSize& size);
    QPixmap getScaledPixmap(const QPixmap& oriPix, qreal scale, bool hasShadow, Qt::TransformationMode transformMode = Qt::FastTransformation);
    void scalePixmap(qreal scale, const QPoint& center = QPoint()); //缩放中心
    void updateInfo(void);
    void adjustBtnPos(void);
    void setIcon(QAbstractButton* btn, const QString& iconName, const QString& suffix = ".ico", const QString& dir = "images"); //default for inner resource
    QPixmap applyEffectToPixmap(const QPixmap& pixmap, QGraphicsEffect* effect, int extent = 0);
    QGraphicsDropShadowEffect* createShadowEffect(int radius, const QPoint& offset = QPoint(0, 0), const QColor& color = QColor(20, 20, 20));
    void setPixmap(const QString& path);
    void updateAll(void);
    qreal scaleToScreen(const QSize& pixSize);
    qreal scaleToScreen(const QPixmap& pixmap);
    QRect getShadowRect(const QRect& rect, int Shadow_R);
    void setCircleMenuActions(void);
    void scaleAndMove(qreal scale, const QPoint& center);
    void initThumbnailBar(void);
    void updateThumbnailPixmap(void);
    QStringList getFileList(QString dir, const QStringList& filter);
    QString getDirPath(const QString& filePath);
    QString getFileName(const QString& filePath);
    int switchPixmap(int i); //在文件夹中切换图片(fileList)

signals:
    void updateSmoothPixmap(QPixmap smoothPixmap, qreal scale);
    void updateRealSizePixmap(QPixmap realPixmap, qreal scale, QString path);

private:
    Ui::Widget* ui;

    QRect pixRect { 50, 50, 0, 0 };
    QPoint curPos;
    bool canMovePix = false;
    QPixmap pixmap; //原始图像
    QPixmap toShow; //缩放的图像(实际显示)
    qreal scaleSize = 1.0; //取名鬼才，很容易让人误以为是size的好吧
    const QPair<int, int> pixelRange { (int)1e3, (int)1e8 };
    const int Shadow_P_Limit = 1.5e6;
    const int Shadow_R = 15;
    const int MenuDelay = 150; //ms
    bool isShadowDrop = true;
    const QString defaultImage = R"(E:\图片(New)\声卡.png)"; //"E:\图片(New)\4-4我是大工人.png"//E:\Qt5.14.2\Projects\ImageViewer_2\default.png
    //E:\图片(New)\表情包\男子高中生的日常.png//E:\图片(New)\声卡.png
    QString ImagePath;
    QScreen* screen;
    WinThumbnailToolBar* thumbbar = nullptr;
    QStringList fileList;
    QString curDirPath;
    int index = -1;
    bool isGif = false;

public:
    static QStringList Filter;

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // QWidget interface
protected:
    void wheelEvent(QWheelEvent* event) override;

    // QWidget interface
protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
};
#endif // WIDGET_H
