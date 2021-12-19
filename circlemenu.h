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
    void appendAction(const QString& text, std::function<void(void)> func);

private:
    Ui::CircleMenu* ui;

    QPoint startPos, endPos;
    bool startMenu = false;
    int highLight = 0;
    const int radius = 75;
    const int safeRadius = 20;
    QList<QPair<QString, std::function<void(void)>>> itemList; //"适应屏幕", "退出", "Bomb", "shutdown", "Noooooooo"

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // CIRCLEMENU_H
