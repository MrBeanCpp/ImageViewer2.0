#ifndef WIDGET_H
#define WIDGET_H

#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00

#include <QAbstractButton>
#include <QGraphicsEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPair>
#include <QRect>
#include <QTime>
#include <QWidget>
#include <QWinThumbnailToolBar>
#include <windows.h>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    bool isOnPixmap(const QPoint& curPos);
    bool isInPixelRange(int pixels);
    void scalePixmap(qreal scale, const QPoint& center = QPoint()); //缩放中心
    void updateInfo(void);
    void adjustBtnPos(void);
    void setIcon(QAbstractButton* btn, const QString& iconName, const QString& suffix = ".ico", const QString& dir = "images"); //default for inner resource
    QPixmap applyEffectToPixmap(const QPixmap& pixmap, QGraphicsEffect* effect, int extent = 0);
    QGraphicsDropShadowEffect* createShadowEffect(int radius, const QPoint& offset = QPoint(0, 0), const QColor& color = QColor(20, 20, 20));
    void setPixmap(const QString& path);
    void updateAll(void);
    qreal scaleToScreen(const QPixmap& pixmap);
    QRect getShadowRect(const QRect& rect, int Shadow_R);
    void setCircleMenuActions(void);
    void scaleAndMove(qreal scale, const QPoint& center);
    void setThumbnailPixmap(const QPixmap& pixmap);
    void setLivePreviewPixmap(const QPixmap& pixmap);
    void initThumbnailBar(void);

private:
    Ui::Widget* ui;

    QRect pixRect { 50, 50, 0, 0 };
    QPoint curPos;
    bool canMovePix = false;
    QPixmap pixmap; //原始图像
    QPixmap toShow; //缩放的图像(实际显示)
    qreal scaleSize = 1.0;
    const QPair<int, int> pixelRange { (int)1e3, (int)1e8 };
    const int Shadow_P_Limit = 1.5e6;
    const int Shadow_R = 15;
    bool isShadowDrop = true;
    const QString defaultImage = R"(E:\Qt5.14.2\Projects\ImageViewer_2\default.png)";
    QString ImagePath;
    QScreen* screen;
    QWinThumbnailToolBar* thumbbar = nullptr;

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    //void keyPressEvent(QKeyEvent* event) override;
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
