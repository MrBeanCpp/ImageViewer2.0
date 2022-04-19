#ifndef CIRCLEMENU_H
#define CIRCLEMENU_H

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>
#include <functional>
namespace Ui {
class CircleMenu;
}

class CircleMenu : public QWidget
{
    Q_OBJECT

public:
    explicit CircleMenu(QWidget *parent = nullptr);
    ~CircleMenu();

    void setStartPos(const QPoint& pos);
    void setEndPos(const QPoint& pos);
    void release(void);
    QRect getTextRect(const QFont& font, const QString& text);
    QRect calcItemRect(int i, QPoint center); //正常高中数学二维坐标系（y上正下负）
    void appendAction(const QString& text, std::function<void(void)> func);
    void calcBtnRectsByDegree(void);
    void calcBtnRectsByHeight(void);
    bool isSelected(void);
    QRect getBoundingRect(void); //废弃 每次move都要运算会增大CPU压力
    void renameAction(const QString& oldName, const QString& newName);

private:
    Ui::CircleMenu* ui;

    QPoint startPos, endPos;
    bool startMenu = false;
    int highLight = -1;
    int radius = 75;
    int safeRadius = 25;
    int limitGap = 25; //btns间最小width间距
    QSize maskSize = { 400, 400 };
    QList<QPair<QString, std::function<void(void)>>> itemList; //"适应屏幕", "退出", "Bomb", "shutdown", "Noooooooo"
    QVector<QRect> btns;
    QFont font { "微软雅黑", 9 };

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // CIRCLEMENU_H
