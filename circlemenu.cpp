#include "circlemenu.h"
#include "ui_circlemenu.h"
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QtMath>
CircleMenu::CircleMenu(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CircleMenu)
{
    ui->setupUi(this);
    hide();
}

CircleMenu::~CircleMenu()
{
    delete ui;
}

void CircleMenu::setStartPos(const QPoint& pos)
{
    endPos = startPos = pos - this->pos();
    startMenu = true;
    update();
    show();
}

void CircleMenu::setEndPos(const QPoint& pos)
{
    endPos = pos - this->pos();
    update();
}

void CircleMenu::release()
{
    startMenu = false;
    update();
    hide();
}

void CircleMenu::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    static QList<QString> list = { "适应屏幕", "退出", "Bomb", "shutdown", "Noooooooo" };
    if (startMenu) {
        painter.setPen(QPen(Qt::black, 1));
        const int n = list.size();
        const int R = 60;
        qreal degree = 90, delta = 360.0 / n;
        QLineF line = QLineF(startPos, endPos);
        qreal lineAngle = line.angle();
        highLight = (int(lineAngle - (90 - delta / 2) + 360) % 360) / delta; //修正为 正数
        if (line.length() < R / 3) highLight = -1; //不可选区域
        for (int i = 0; i < n; i++, degree += delta) {
            int x = startPos.x() + qCos(qDegreesToRadians(degree)) * R;
            int y = startPos.y() - qSin(qDegreesToRadians(degree)) * R;
            if (i == highLight)
                painter.setBrush(QColor(50, 100, 220));
            else
                painter.setBrush(QColor(50, 50, 50));
            QRect rect(QRect(x, y, 0, 0).marginsAdded(QMargins(25, 10, 25, 10)));
            painter.drawRect(rect);
            painter.setPen(QPen(Qt::white, 1));
            painter.drawText(rect, Qt::AlignCenter, list[i]);
        }

        if (startPos != endPos) { //连结鼠标的Line
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::black, 1));
            painter.drawEllipse(startPos, 4, 4);
            painter.drawRect(QRect(endPos - QPoint(4, 4), QSize(8, 8)));
            painter.setPen(QPen(Qt::black, 2));
            painter.drawLine(startPos, endPos);
        }
    }
}
