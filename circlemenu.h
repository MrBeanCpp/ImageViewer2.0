#ifndef CIRCLEMENU_H
#define CIRCLEMENU_H

#include <QWidget>

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

private:
    Ui::CircleMenu* ui;

    QPoint startPos, endPos;
    bool startMenu = false;
    int highLight = 0;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif // CIRCLEMENU_H
