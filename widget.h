#ifndef WIDGET_H
#define WIDGET_H

#include <QRect>
#include <QWidget>
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
    void scalePixmap(qreal scale, const QPoint& center); //缩放中心

private:
    Ui::Widget* ui;

    QRect pixRect { 50, 50, 0, 0 };
    QPoint curPos;
    QPixmap pixmap;
    QPixmap toShow;
    qreal scaleSize = 1.0;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // QWidget interface
protected:
    void wheelEvent(QWheelEvent* event) override;
};
#endif // WIDGET_H
