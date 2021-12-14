#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , screen(qApp->screens().at(0))
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); //启用半透明背景//也可以实现背景穿透！！！

    this->setFocusPolicy(Qt::StrongFocus);
    move(0, 0);
    //showFullScreen();
    //setWindowState(Qt::WindowMaximized); //对无边框窗口无效
    setFixedSize(screen->geometry().size() - QSize(0, 1)); //留1px 便于触发任务栏（自动隐藏）

    ui->circleMenu->setFixedSize(size());
    ui->circleMenu->move(0, 0);
    ui->circleMenu->hide();

    ui->label_info->move(20, -1);

    ui->btn_info->setFocusProxy(this); //委托焦点，防止点击按钮 label焦点丢失
    ui->btn_pin->setFocusProxy(this);

    connect(ui->btn_pin, &QPushButton::clicked, [=](bool checked) { //前置
        SetWindowPos(HWND(winId()), checked ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        setIcon(ui->btn_pin, checked ? "pin_on" : "pin_off");
    });

    connect(ui->btn_info, &QPushButton::clicked, [=]() { //打开文件夹并选中
        ShellExecuteW(NULL, L"open", L"explorer", QString("/select, \"%1\"").arg(ImagePath).toStdWString().c_str(), NULL, SW_SHOW);
    });

    QStringList args = qApp->arguments();
    ImagePath = args.size() > 1 ? args.at(1) : defaultImage;

    setPixmap(ImagePath);
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::isOnPixmap(const QPoint& curPos)
{
    return pixRect.contains(curPos);
}

bool Widget::isInPixelRange(int pixels)
{
    return pixels >= pixelRange.first && pixels <= pixelRange.second;
}

void Widget::scalePixmap(qreal scale, const QPoint& center)
{
    QSize newSize = pixmap.size() * scale;
    int pixels = newSize.width() * newSize.height();
    if (isInPixelRange(pixels)) { //限制像素范围，使用长宽不太准确
        QPoint oldCurPos = center - pixRect.topLeft(); //relative
        QPoint newCurPos = oldCurPos * (scale / scaleSize);
        toShow = pixmap.scaled(newSize);
        if (pixels <= Shadow_P_Limit) { //究极优化（图片太大 计算阴影卡顿）
            toShow = applyEffectToPixmap(toShow, createShadowEffect(Shadow_R), Shadow_R);
            isShadowDrop = true;
        } else
            isShadowDrop = false;
        pixRect.translate(oldCurPos - newCurPos);
        pixRect.setSize(newSize);
        scaleSize = scale;

        updateAll();
    }
}

void Widget::updateInfo()
{
    ui->label_info->setText(QString("  W: %1  H: %2  Pixel: %3  Scale: %4%  ").arg(pixRect.width()).arg(pixRect.height()).arg(pixRect.width() * pixRect.height()).arg(scaleSize * 100, 0, 'f', 0));
    ui->label_info->adjustSize();
}

void Widget::adjustBtnPos()
{
    ui->Btns->move(pixRect.left(), pixRect.bottom());
}

void Widget::setIcon(QAbstractButton* btn, const QString& iconName, const QString& suffix, const QString& dir)
{
    if (btn == nullptr) return;
    QString path = ":/" + dir + "/" + iconName + suffix;
    btn->setIcon(QIcon(path));
}

QPixmap Widget::applyEffectToPixmap(const QPixmap& pixmap, QGraphicsEffect* effect, int extent)
{
    if (pixmap.isNull()) return pixmap;

    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(pixmap);
    item.setGraphicsEffect(effect);
    scene.addItem(&item);
    QPixmap res(pixmap.size() + QSize(extent * 2, extent * 2));
    res.fill(Qt::transparent);
    QPainter ptr(&res);
    scene.render(&ptr, QRectF(), QRectF(-extent, -extent, pixmap.width() + extent * 2, pixmap.height() + extent * 2));
    return res;
}

QGraphicsDropShadowEffect* Widget::createShadowEffect(int radius, const QPoint& offset, const QColor& color)
{
    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(radius);
    effect->setColor(color);
    effect->setOffset(offset);
    return effect;
}

void Widget::setPixmap(const QString& path)
{
    ImagePath = path;
    pixmap = QPixmap(path);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Warning", "Error File Path!\n错误文件路径\n間違えたファイルパス");
        QTimer::singleShot(0, [=]() { qApp->quit(); }); //需要进入事件循环后触发
        return;
    }
    scaleSize = qMin(scaleToScreen(pixmap), 1.0); //缩放到能完全显示
    pixRect.setSize(pixmap.size() * scaleSize);
    pixRect.moveCenter(this->rect().center());

    toShow = applyEffectToPixmap(pixmap.scaled(pixRect.size()), createShadowEffect(Shadow_R), Shadow_R);

    ui->btn_info->setToolTip(QFileInfo(path).fileName() + " [Click to Open]");

    updateAll();
}

void Widget::updateAll()
{
    updateInfo();
    adjustBtnPos();
    update();
    //this->setMask(toDrawRegion());//update并非实时生效，所以得放在paintEvent中确保Mask & painter同时生效
}

QRegion Widget::toDrawRegion()
{
    const int Extend = Shadow_R + 25; //防止残影
    QRegion res = pixRect.marginsAdded(QMargins(Extend, Extend, Extend, Extend));
    res += ui->Btns->geometry();
    res += ui->label_info->geometry();
    return res.intersected(this->rect());
}

qreal Widget::scaleToScreen(const QPixmap& pixmap)
{
    const QSize sSize = screen->size();
    const QSize pixSize = pixmap.size();
    //if (sSize.width() > pixSize.width() && sSize.height() > pixSize.height()) return 1.0;
    qreal sW = (qreal)sSize.width() / pixSize.width();
    qreal sH = (qreal)sSize.height() / pixSize.height();
    return qMin(sW, sH);
}

void Widget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QPoint startPos = pixRect.topLeft();
    if (isShadowDrop) startPos -= QPoint(Shadow_R, Shadow_R);
    painter.drawPixmap(startPos, toShow);

    this->clearMask(); //防止mask更新不及时 导致显示不完全
    if (ui->circleMenu->isHidden()) //防止截断circleMenu
        this->setMask(toDrawRegion());
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    curPos = event->globalPos();
    if (!isOnPixmap(curPos)) return; //防止鼠标在其他部分移动
    if (event->button() == Qt::LeftButton)
        canMovePix = true;
    else if (event->button() == Qt::RightButton) { //延迟触发：长按为Menu 短按为quit
        QTimer::singleShot(100, [=]() { ui->circleMenu->setStartPos(curPos); });
    }
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        if (ui->circleMenu->isVisible()) //延迟触发：长按为Menu 短按为quit
            ui->circleMenu->release();
        else
            qApp->quit();
    } else if (event->button() == Qt::LeftButton)
        canMovePix = false;
}

void Widget::mouseMoveEvent(QMouseEvent* event) //破案了 透明窗体 会让mouseMoveEvent卡顿（与其Size有关）//通过设置窗口Mask的方法解决(减少面积)
{
    if ((event->buttons() & Qt::LeftButton) && canMovePix) {
        QPoint mousePos = event->globalPos();
        QPoint newPos = pixRect.topLeft() + mousePos - curPos;
        curPos = mousePos;
        pixRect.moveTopLeft(newPos);

        updateAll();
    } else if (event->buttons() & Qt::RightButton) {
        ui->circleMenu->setEndPos(event->globalPos());
    }
}

void Widget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return; //过滤自动触发

    switch (event->key()) {
    case Qt::Key_Space: //100%
        scalePixmap(1.0, QCursor::pos());
        pixRect.moveCenter(this->rect().center());
        updateAll();
        break;
    case Qt::Key_Backspace: //适应屏幕
        scalePixmap(scaleToScreen(pixmap), QCursor::pos());
        pixRect.moveCenter(this->rect().center());
        updateAll();
        break;
    case Qt::Key_Escape:
        qApp->quit();
        break;
    default:
        break;
    }
}

void Widget::wheelEvent(QWheelEvent* event)
{
    qreal scale = event->delta() > 0 ? scaleSize * 1.1 : scaleSize / 1.1; //观察微软原生Photo得出结论
    scalePixmap(scale, event->pos());
}

void Widget::focusInEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    ui->label_info->show();
    ui->Btns->show();
}

void Widget::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    ui->label_info->hide();
    ui->Btns->hide();
}
