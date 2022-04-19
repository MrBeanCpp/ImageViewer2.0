#include "circlemenu.h"
#include "ui_circlemenu.h"
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QtMath>
CircleMenu::CircleMenu(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CircleMenu)
{
    ui->setupUi(this);
    hide();

    QScreen* screen = qApp->screens().at(0);
    int len = qMin(screen->size().width(), screen->size().height()); //取短边 防止竖屏
    qreal DPIscale = 1.0 * len / 1080; //1080p //高DPI适配
    radius *= DPIscale;
    safeRadius *= DPIscale;
    maskSize *= DPIscale;
    limitGap *= DPIscale;

    //    appendAction("Text", [=]() { qDebug() << "Test"; });
    //    appendAction("Text2", [=]() { qDebug() << "Test2"; });
}

CircleMenu::~CircleMenu()
{
    delete ui;
}

void CircleMenu::setStartPos(const QPoint& pos)
{
    endPos = startPos = pos - this->pos();
    if (itemList.size() < 5) //重新计算位置
        calcBtnRectsByDegree(); //by角度间隔
    else
        calcBtnRectsByHeight(); //by高度间隔

    QRect Mask(QPoint(), maskSize);
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

QRect CircleMenu::calcItemRect(int i, QPoint center)
{
    QRect rect = getTextRect(font, itemList[i].first).marginsAdded(QMargins(8, 5, 8, 5));
    center.setY(-center.y()); //转换坐标系（原本是高中数学二维坐标轴）
    rect.moveCenter(center + startPos);
    return rect;
}

void CircleMenu::appendAction(const QString& text, std::function<void(void)> func) //添加button
{
    itemList << qMakePair(text, func); //第一个参数不能直接用"text" 字符串字面量会干扰template判断类型
}

void CircleMenu::calcBtnRectsByDegree() //计算按钮位置
{
    const int n = itemList.size();
    qreal degree = 90, delta = 360.0 / n;
    btns.clear();

    for (int i = 0; i < n; i++, degree += delta) { //获取Btn Rect
        int x = qCos(qDegreesToRadians(degree)) * radius;
        int y = qSin(qDegreesToRadians(degree)) * radius;
        btns << calcItemRect(i, QPoint(x, y));
    }
}

void CircleMenu::calcBtnRectsByHeight()
{
    const int n = itemList.size();
    btns.clear();
    btns.resize(n); //QList不能用resize，而reserve不会初始化

    btns[0] = calcItemRect(0, QPoint(0, radius)); //top

    const int Level = n / 2;
    qreal deltaH = (2.0 * radius) / Level; //高度间隔
    if (n & 1) {
        assert(limitGap < radius); //计算最底层两个btns的间距来调整高度
        int limitW = calcItemRect(Level, QPoint()).width() / 2 + calcItemRect(n - Level, QPoint()).width() / 2 + limitGap;
        deltaH = (sqrt(qPow(radius, 2) - qPow(limitW / 2, 2)) + radius) / Level;
    }

    for (int i = 1; i <= Level; i++) {
        qreal h = radius - deltaH * i;
        qreal w = sqrt(radius * radius - h * h);

        btns[i] = calcItemRect(i, QPoint(-w, h));
        if (!qFuzzyCompare(w, 0)) //最底层
            btns[n - i] = calcItemRect(n - i, QPoint(w, h)); //镜像右侧
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

void CircleMenu::renameAction(const QString& oldName, const QString& newName)
{
    for (auto& pair : itemList)
        if (pair.first == oldName) {
            pair.first = newName;
            return;
        }
}

void CircleMenu::paintEvent(QPaintEvent* event) //不应该根据相同角度分配Rect 而是要保持间隔高度相等以及两个Rect之间的最小width！！！！！！！圆上的分布问题
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
