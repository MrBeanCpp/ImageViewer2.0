#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPainter>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); //启用半透明背景//也可以实现背景穿透！！！
    move(0, 0);
    showFullScreen();
    toShow = pixmap = QPixmap("E:\\Qt5.14.2\\Projects\\ImageViewer_2\\default.png"); //"E:\\图片(New)\\Mirror.png"    //"E:\\Qt5.14.2\\Projects\\ImageViewer_2\\default.png"
    pixRect.setSize(pixmap.size());
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::isOnPixmap(const QPoint& curPos)
{
    return pixRect.contains(curPos);
}

void Widget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    //QRect source(20, 20, 500, 500);
    static QPen pen;
    pen.setWidth(4);
    pen.setJoinStyle(Qt::MiterJoin); //使矩形边角尖锐
    painter.setPen(pen);
    painter.drawRect(pixRect);
    painter.drawPixmap(pixRect.topLeft(), toShow);
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        curPos = event->pos();
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
        qApp->quit();
}

void Widget::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) return;
    QPoint mousePos = event->pos();
    QPoint newPos = pixRect.topLeft() + mousePos - curPos;
    curPos = mousePos;
    pixRect.moveTopLeft(newPos);
    update();
}

void Widget::wheelEvent(QWheelEvent* event)
{
    qreal oldScaleSize = scaleSize;
    scaleSize += event->delta() > 0 ? 0.1 : -0.1;
    QSize newSize = pixmap.size() * scaleSize;
    QPoint oldCurPos = event->pos() - pixRect.topLeft(); //relative
    QPoint newCurPos = oldCurPos * (scaleSize / oldScaleSize);
    toShow = pixmap.scaled(newSize);
    pixRect.translate(oldCurPos - newCurPos);
    pixRect.setSize(newSize);
    update();
}
