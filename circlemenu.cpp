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
    calcBtnRects(); //重新计算位置

    QRect Mask(QPoint(), QSize(400, 400));
    Mask.moveCenter(startPos);
    setMask(Mask); //减少CPU压力

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
    if (isSelected())
        itemList[highLight].second(); //调用关联函数

    startMenu = false;
    update();
    hide();
}

QRect CircleMenu::getTextRect(const QFont& font, const QString& text)
{
    return QFontMetrics(font).boundingRect(text);
}

void CircleMenu::appendAction(const QString& text, std::function<void(void)> func) //添加button
{
    itemList << qMakePair(text, func); //第一个参数不能直接用"text" 字符串字面量会干扰template判断类型
}

void CircleMenu::calcBtnRects() //计算按钮位置
{
    const int n = itemList.size();
    qreal degree = 90, delta = 360.0 / n;
    btns.clear();

    for (int i = 0; i < n; i++, degree += delta) { //获取Btn Rect
        int x = startPos.x() + qCos(qDegreesToRadians(degree)) * radius;
        int y = startPos.y() - qSin(qDegreesToRadians(degree)) * radius;
        QRect rect = getTextRect(font, itemList[i].first).marginsAdded(QMargins(8, 5, 8, 5));
        rect.moveCenter(QPoint(x, y));
        btns << rect;
    }
}

bool CircleMenu::isSelected()
{
    return highLight >= 0;
}

QRect CircleMenu::getBoundingRect() //获取整个CircleMenu的bounding Rect //废弃 每次move都要运算会增大CPU压力
{
    static QRect endPosRect(QPoint(), QSize(12, 12));
    endPosRect.moveCenter(endPos);
    QLineF line = QLineF(startPos, endPos);
    QRect curRect(QPoint(), QSize(qAbs(line.dx()) * 2 + 5, qAbs(line.dy()) * 2 + 5));
    curRect.moveCenter(startPos);
    QRegion region(curRect);
    for (auto& rect : btns)
        region += rect.marginsAdded(QMargins(2, 2, 2, 2));
    region += endPosRect;
    return region.boundingRect();
}

void CircleMenu::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(font);
    if (startMenu) {
        const int n = itemList.size();
        qreal delta = 360.0 / n;
        QLineF line = QLineF(startPos, endPos);
        qreal lineAngle = line.angle();
        highLight = (int(lineAngle - (90 - delta / 2) + 360) % 360) / delta; //修正为 正数 //选中区域
        if (line.length() < safeRadius) highLight = -1; //不可选区域

        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(QColor(50, 50, 50));
        for (auto& rect : btns) //绘制Btns
            painter.drawRect(rect);

        if (isSelected()) { //绘制选中区域
            painter.setBrush(QColor(50, 100, 210));
            painter.drawRect(btns[highLight]);
        }

        painter.setPen(QPen(Qt::white, 1));
        for (int i = 0; i < n; i++) //绘制Text
            painter.drawText(btns[i], Qt::AlignCenter, itemList[i].first);

        if (isSelected()) { //连结鼠标的Line
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::black, 1));
            painter.drawEllipse(startPos, 4, 4);
            painter.drawRect(QRect(endPos - QPoint(4, 4), QSize(8, 8)));
            painter.setPen(QPen(Qt::gray, 3));
            painter.drawLine(startPos, endPos);
        }
    }
}
