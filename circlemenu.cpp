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

    //appendAction("Text", [=]() { qDebug() << "Test"; });
    //appendAction("Text2", [=]() { qDebug() << "Test2"; });
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
    if (highLight >= 0)
        itemList[highLight].second();

    startMenu = false;
    update();
    hide();
}

QRect CircleMenu::getTextRect(const QFont& font, const QString& text)
{
    return QFontMetrics(font).boundingRect(text);
}

void CircleMenu::appendAction(const QString& text, std::function<void(void)> func)
{
    itemList << qMakePair(text, func); //第一个参数不能直接用"text" 字符串字面量会干扰template判断类型
}

void CircleMenu::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (startMenu) {
        const int n = itemList.size();
        qreal degree = 90, delta = 360.0 / n;
        QLineF line = QLineF(startPos, endPos);
        qreal lineAngle = line.angle();
        highLight = (int(lineAngle - (90 - delta / 2) + 360) % 360) / delta; //修正为 正数 //选中区域
        if (line.length() < safeRadius) highLight = -1; //不可选区域
        QList<QRect> btns;

        for (int i = 0; i < n; i++, degree += delta) { //获取Btn Rect
            int x = startPos.x() + qCos(qDegreesToRadians(degree)) * radius;
            int y = startPos.y() - qSin(qDegreesToRadians(degree)) * radius;
            QRect rect = getTextRect(painter.font(), itemList[i].first).marginsAdded(QMargins(8, 5, 8, 5));
            rect.moveCenter(QPoint(x, y));
            btns << rect;
        }

        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(QColor(50, 50, 50));
        for (auto& rect : btns) //绘制Btns
            painter.drawRect(rect);

        if (highLight >= 0) { //绘制选中区域
            painter.setBrush(QColor(50, 100, 220));
            painter.drawRect(btns[highLight]);
        }

        painter.setPen(QPen(Qt::white, 1));
        for (int i = 0; i < n; i++) //绘制Text
            painter.drawText(btns[i], Qt::AlignCenter, itemList[i].first);

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
