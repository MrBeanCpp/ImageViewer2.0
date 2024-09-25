#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
//#include <QMutex>
#include <QMenu>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QtConcurrent>
#include <QtWinExtras>
#include "util.h"
#include "winEventHook.h"


QStringList Widget::Filter = {"*.png", "*.jpg", "*.bmp", "*.gif", "*.jpeg"};

Widget::Widget(QWidget* parent)
    : QWidget(parent), ui(new Ui::Widget), screen(qApp->screens().at(0))
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); //启用半透明背景//也可以实现背景穿透！！！

    setWindowTitle("ImageViewer2.0");
    this->setFocusPolicy(Qt::StrongFocus);
    move(0, 0);
    //showFullScreen();
    //setWindowState(Qt::WindowMaximized); //对无边框窗口无效
    setFixedSize(screen->geometry().size() - QSize(0, 1)); //留1px 便于触发任务栏（自动隐藏）

    QTimer::singleShot(0, this, [=]() { initThumbnailBar(); }); //必须在窗口显示后(构造完成后) or Handle == 0

    setCircleMenuActions();

    ui->circleMenu->setFixedSize(size());
    ui->circleMenu->move(0, 0);
    ui->circleMenu->hide();

    ui->label_info->move(20, -1);

    ui->label_tip->setGraphicsEffect(createShadowEffect(10));
    ui->label_tip->hide();

    ui->label_image->setAttribute(Qt::WA_TransparentForMouseEvents); //鼠标穿透 父窗口处理鼠标事件

    ui->btn_info->setFocusProxy(this); //委托焦点，防止点击按钮 label焦点丢失
    ui->btn_pin->setFocusProxy(this);

    ui->label_version->setText(QString("  Version: [%1]  by MrBeanC  ").arg(qApp->applicationVersion()));
    ui->label_version->adjustSize();
    QRect verRect = ui->label_version->geometry();
    verRect.moveBottomRight(screen->availableGeometry().bottomRight());
    ui->label_version->setGeometry(verRect);
    ui->label_version->hide();

    // This may be the result of a user action, click() slot activation, or because setChecked() is called.
    // 可以被click() & setChecked()激活
    connect(ui->btn_pin, &QPushButton::toggled, this, [=](bool checked) {
        setIcon(ui->btn_pin, checked ? "pin_on" : "pin_off");
        if (checked) {
            ui->circleMenu->renameAction("Set 置顶", "取消置顶");
        }
        else {
            ui->circleMenu->renameAction("取消置顶", "Set 置顶");
        }
    });

    // 只能被click()激活
    connect(ui->btn_pin, &QPushButton::clicked, this, [=](bool checked) { //全局置顶
        Util::setWindowTopMost(this, checked);
        showTip(QString("Top Mode %1").arg(checked ? "ON" : "OFF"));
        if (!checked) {
            unhookWinEvent();
            targetWindow = nullptr;
        }
    });

    connect(qApp, &QApplication::aboutToQuit, this, []() {
        unhookWinEvent();
    });

    connect(ui->btn_info, &QPushButton::clicked, this, [=]() { //打开文件夹并选中
        ShellExecuteW(NULL, L"open", L"explorer", QString("/select, \"%1\"").arg(QDir::toNativeSeparators(ImagePath)).toStdWString().c_str(), NULL, SW_SHOW);
    });

    connect(ui->btn_rotate, &QPushButton::clicked, this, [=]() { //旋转并保存
        rotateClockwise();
        pixmap.save(ImagePath);
    });

    connect(
        this, &Widget::updateSmoothPixmap, this, [=](const QPixmap& smoothPix, qreal scale) {
            if (scale != this->scaleSize) return; //[写在此处而非子线程] 判断更准确(更接近setPixmap)
            ui->label_image->setPixmap(toShow = smoothPix); //多线程or非GUI访问出错(有互斥锁貌似也不行) // & 大图片耗时大概20ms
        },
        Qt::QueuedConnection); //多线程（重要）

    connect(
        this, &Widget::updateRealSizePixmap, this, [=](const QPixmap& realPix, qreal scale, const QString& path) {
            if (path != this->ImagePath) return; //若已切换图片 则作废
            qDebug() << "updateRealSizePixmap";
            toShow = getScaledPixmap(pixmap = realPix, scaleSize *= scale, isShadowDrop, Qt::SmoothTransformation); //注意scaleSize更新
            updateAll();
        },
        Qt::QueuedConnection);

    connect(this, &Widget::menuRequested, [=]() {
        isMenuRequested = true;
        ui->circleMenu->setStartPos(curPos);
        ui->label_version->show();
    });
    connect(this, &Widget::menuCloseRequested, [=]() {
        isMenuRequested = false;
        ui->circleMenu->release();
        ui->label_version->hide();
    });

    QStringList args = qApp->arguments();
    ImagePath = args.size() > 1 ? args.at(1) : defaultImage;

    QtConcurrent::run([=]() { //多线程加载文件夹图片列表
        fileList = getFileList(ImagePath, Filter);
        index = fileList.indexOf(Util::getFileName(ImagePath));
        qDebug() << "fileList Loaded"; //100 files 5ms
    });

    setPixmap(ImagePath);

    setFocus(); //否则首次按键不响应
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::isOnPixmap(const QPoint& curPos)
{
    return pixRect.contains(curPos);
}

int Widget::getPixels(const QSize& size)
{
    return size.width() * size.height();
}

bool Widget::isInPixelRange(int pixels)
{
    return pixels >= pixelRange.first && pixels <= pixelRange.second;
}

bool Widget::isInPixelRange(const QSize& size)
{
    return isInPixelRange(getPixels(size));
}

QPixmap Widget::getScaledPixmap(const QPixmap& oriPix, qreal scale, bool hasShadow, Qt::TransformationMode transformMode)
{
    QSize newSize = oriPix.size() * scale; //↓ 1.0优化
    QPixmap resPix = qFuzzyCompare(scale, 1.0) ? oriPix : oriPix.scaled(newSize, Qt::IgnoreAspectRatio, transformMode); //Qt::KeepAspectRatio效率较差
    if (hasShadow) resPix = applyEffectToPixmap(resPix, createShadowEffect(Shadow_R), Shadow_R); // 给控件添加QGraphicsDropShadowEffect：在坐标为负时报错，所以先生成阴影pixmap再显示图片
    return resPix;
}

void Widget::scalePixmap(qreal scale, const QPoint& center)
{
    QSize newSize = pixmap.size() * scale;
    if (isInPixelRange(newSize)) { //限制像素范围，使用长宽不太准确
        QPoint oldCurPos = center - pixRect.topLeft(); //relative
        QPoint newCurPos = oldCurPos * (scale / scaleSize);

        isShadowDrop = (getPixels(newSize) <= Shadow_P_Limit); //究极优化（图片太大 计算阴影卡顿）
        if (!isGif) { //非Gif 计算pixmap
            QtConcurrent::run([=]() { //多线程获取平滑图像，但是对于GUI的操作还得在GUI线程完成，所以emit signal
                QPixmap smoothPix = getScaledPixmap(pixmap, scale, isShadowDrop, Qt::SmoothTransformation);
                emit updateSmoothPixmap(smoothPix, scale);
            }); //开启子线程 GUI线程耗时0ms 这句在前
            toShow = getScaledPixmap(pixmap, scale, isShadowDrop); //大图耗时100-200ms
        }
        pixRect.translate(oldCurPos - newCurPos);
        pixRect.setSize(newSize);
        scaleSize = scale;

        updateAll();
    }
}

void Widget::updateInfo()
{
    updateInfo(QString("  W: %1  H: %2  Pixel: %3  Scale: %4%  |  [  %5  ]  ")
                                .arg(pixRect.width())
                                .arg(pixRect.height())
                                .arg(pixRect.width() * pixRect.height())
                                .arg(scaleSize * 100, 0, 'f', 0)
                                .arg(Util::getFileName(ImagePath)));
}

void Widget::updateInfo(const QString& str)
{
    ui->label_info->setText(str);
    ui->label_info->adjustSize(); //自适应大小
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
    // setGraphicsEffect会 take ownership，所以不需要delete
    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(); //不能写parent == this 否则不能多线程//QObject: Cannot create children for a parent that is in a different thread.
    effect->setBlurRadius(radius);
    effect->setColor(color);
    effect->setOffset(offset);
    return effect;
}

void Widget::setPixmap(const QString& path)
{
    ImagePath = path;
    QImageReader reader(path);
    reader.setAutoTransform(true); //根据图片的EXIF信息自动调整图片的方向，包括旋转和翻转
    if (reader.canRead() == false) {
        QMessageBox::warning(this, "Warning", "Can not read!\n无法读取文件（路径|格式问题）\nしまった");
        QTimer::singleShot(0, this, [=]() { qApp->quit(); }); //需要进入事件循环后触发
        return;
    }
    isGif = (QFileInfo(path).suffix().toLower() == "gif"); //or reader.imageCount()>1

    //reader.size();返回的是原始Size（没有自动应用旋转）
    QSize realSize = getTransformedSize(reader); //without Much I/O
    qreal realScale = qMin(scaleToScreen(realSize), 1.0); //缩放到能完全显示
    QSize fitSize = realSize * realScale;
    bool needSwapSize = reader.transformation() & QImageIOHandler::TransformationRotate90;
    reader.setScaledSize(needSwapSize ? swapSize(fitSize) : fitSize); //Scale应用于自动旋转之前，所以得是原始Size
    scaleSize = 1.0;

    if (!isGif && !qFuzzyCompare(realScale, scaleSize)) //realScale != scaleSize(1.0)意味着图片经过缩放（大于屏幕）
        QtConcurrent::run([=]() { //多线程加载真实大小图片
            QImageReader reader(path);
            reader.setAutoTransform(true); //自动旋转图片
            QPixmap realPix = QPixmap::fromImageReader(&reader);
            emit updateRealSizePixmap(realPix, realScale, path);
        });

    QElapsedTimer t;
    t.start();
    pixmap = QPixmap::fromImageReader(&reader); //如果是.gif则是第一帧图像
    qDebug() << "Load:" << t.elapsed() << "ms";

    if (isGif) { //.gif
        static QMovie* movie = nullptr; //delete nullptr SAFE
        delete movie; //label does NOT take ownership 需要手动delete(如果多次加载gif的话)
        movie = new QMovie(path, QByteArray(), this);
        ui->label_image->setMovie(movie);
        movie->start();
    } else {
        toShow = getScaledPixmap(pixmap, scaleSize, isShadowDrop = (getPixels(fitSize) <= Shadow_P_Limit), Qt::SmoothTransformation);
    }

    ui->btn_rotate->setEnabled(!isGif);
    ui->label_image->setScaledContents(isGif); //效率低//只在Gif时开启
    ui->btn_info->setToolTip(QDir::toNativeSeparators(path) + " [Click to Open]");

    pixRect.setSize(fitSize);
    pixRect.moveCenter(this->rect().center());

    updateThumbnailPixmap(); //通知DWM缩略图失效，下次需要缩略图时(鼠标移至任务栏图标，而非立即)会重新获取
    updateAll();
}

void Widget::updateAll()
{
    updateInfo();
    adjustBtnPos();

    if (isGif) {
        ui->label_image->setGeometry(pixRect);
    } else {
        ui->label_image->setGeometry(getShadowRect(pixRect, Shadow_R));
        ui->label_image->setPixmap(toShow); //只是个载体
    }
}

qreal Widget::scaleToScreen(const QSize& pixSize)
{
    const QSize sSize = screen->size();
    qreal sW = (qreal)sSize.width() / pixSize.width();
    qreal sH = (qreal)sSize.height() / pixSize.height();
    return qMin(sW, sH);
}

qreal Widget::scaleToScreen(const QPixmap& pixmap)
{
    return scaleToScreen(pixmap.size());
}

// 获取自动旋转之后的Size
QSize Widget::getTransformedSize(const QImageReader& reader)
{
    QSize originalSize = reader.size(); // 获取原始尺寸 //without Much I/O
    QImageIOHandler::Transformations transformation = reader.transformation(); // 获取变换信息
    qDebug() << "Transformation:" << transformation;
    // 如果图像被旋转了90或270度，宽度和高度会互换
    if (transformation & QImageIOHandler::TransformationRotate90) {
        return swapSize(originalSize);
    }

    return originalSize; // 如果没有旋转，或者旋转了180度，尺寸不变
}

QSize Widget::swapSize(const QSize& originalSize)
{
    return QSize(originalSize.height(), originalSize.width());
}

QRect Widget::getShadowRect(const QRect& rect, int Shadow_R)
{
    QRect shadowRect = rect.marginsAdded(QMargins(Shadow_R, Shadow_R, Shadow_R, Shadow_R));
    return isShadowDrop ? shadowRect : rect;
}

void Widget::setCircleMenuActions()
{
    ui->circleMenu->appendAction("打开文件位置", [=]() {
        ui->btn_info->click();
    });
    ui->circleMenu->appendAction("适应屏幕", [=]() {
        scaleAndMove(scaleToScreen(pixmap), this->rect().center());
    });
    ui->circleMenu->appendAction("100%", [=]() {
        scaleAndMove(1.0, this->rect().center());
    });
    ui->circleMenu->appendAction("旋转90°", [=]() { // 左下角的旋转按钮是"旋转并保存"，右键菜单是"仅旋转"
        rotateClockwise();
    });
    ui->circleMenu->appendAction("Quit", [=]() {
        qApp->quit();
    });
    ui->circleMenu->appendAction("Set 置顶", [=]() {
        if (isTopMode()) { // 如果已经置顶，就直接取消置顶
            ui->btn_pin->click();
            return;
        }

        static QMenu* menu = [this]() -> QMenu* {
            QMenu* menu = new QMenu(this);
            // 修复Menu圆角后无法透明的问题
            menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
            menu->setAttribute(Qt::WA_TranslucentBackground); //背景透明

            QAction *globalTop = new QAction("全局置顶", menu);
            QAction *relTop = new QAction("相对置顶（to某窗口）", menu);

            connect(globalTop, &QAction::triggered, this, [=]() {
                ui->btn_pin->click();
            });
            connect(relTop, &QAction::triggered, this, [=]() {
                showTip("Click on a Window to attach.", 10000);
                ui->btn_pin->setChecked(true);
                Util::setWindowTopMost(this, true); // 先置顶一下 防止提示Tip看不到

                setWinEventHook([this](DWORD event, HWND hwnd) {
                    // 排除自身
                    if (hwnd == HWND(this->winId())) return;

                    static QPoint lastTargetPos;
                    if (event == EVENT_SYSTEM_FOREGROUND) { // 前台窗口变化
                        if (targetWindow == nullptr) {
                            targetWindow = hwnd;
                            lastTargetPos = Util::getWindowPos(targetWindow);
                            showTip(QString("Always on top of %1").arg(Util::getProcessDescription(hwnd)), 2000);
                        } else {
                            // qDebug() << "Foreground changed."  << Util::getWindowText(hwnd);
                            Util::setWindowTopMost(this, hwnd == targetWindow);

                            if (hwnd != targetWindow) // 设置其他窗口为TOP，防止被自身覆盖（层级：NOTOPMOST == TOP）
                                Util::setWindowTop(hwnd);
                        }
                    } else if (hwnd == targetWindow) { // 目标窗口事件
                        if (event == EVENT_SYSTEM_MINIMIZESTART) {
                            this->showMinimized(); // 同步跟随最小化
                        } else if (event == EVENT_SYSTEM_MINIMIZEEND) {
                            this->showNormal();
                        } else if (event == EVENT_OBJECT_DESTROY) { // 窗口关闭
                            // 取消置顶
                            if (isTopMode()) {
                                ui->btn_pin->click();
                            }
                        } else if (event == EVENT_OBJECT_LOCATIONCHANGE) { // 窗口位置改变
                            if (!IsIconic(hwnd)) { // 最小化时 位置为(-32000 -32000)，需要过滤
                                QPoint pos = Util::getWindowPos(hwnd);

                                QPoint offset = pos - lastTargetPos;
                                pixRect.moveTopLeft(pixRect.topLeft() + offset);
                                lastTargetPos = pos;
                                updateAll();
                            }
                        }
                    }
                });
            });

            menu->addAction(globalTop);
            menu->addAction(relTop);

            return menu;
        }();

        menu->exec(QCursor::pos());
    });
}

void Widget::scaleAndMove(qreal scale, const QPoint& center)
{
    scalePixmap(scale, QCursor::pos());
    pixRect.moveCenter(center);
    updateAll();
}

void Widget::initThumbnailBar()
{
    if (thumbbar == nullptr)
        thumbbar = new WinThumbnailToolBar(this);

    if (thumbbar->window() == nullptr) {
        thumbbar->setWindow(windowHandle()); //必须在窗口显示后(构造完成后) or Handle == 0
        thumbbar->setIconicPixmapNotificationsEnabled(true); //进行一些属性设置，否则不能设置缩略图
    }
    connect(thumbbar, &WinThumbnailToolBar::thumbnailRequested, this, [=](const QSize& size) {
        thumbbar->setThumbnailPixmap(pixmap, size);
    });
    connect(thumbbar, &WinThumbnailToolBar::iconicLivePreviewPixmapRequested, this, [=]() {
        thumbbar->setIconicLivePreviewPixmap(this->grab());
    });
}

void Widget::updateThumbnailPixmap()
{
    if (thumbbar == nullptr || thumbbar->window() == nullptr) return; //若引用空指针，会异常退出
    thumbbar->updateThumbnailPixmap();
}

QStringList Widget::getFileList(QString dir, const QStringList& filter)
{
    if (dir.isEmpty()) return QStringList();
    if (!QFileInfo(dir).isDir()) dir = Util::getDirPath(dir);
    this->curDirPath = dir;

    return QDir(dir).entryList(filter, QDir::Files | QDir::NoSymLinks, QDir::LocaleAware);
}

int Widget::switchPixmap(int i)
{
    if (fileList.isEmpty()) return -1;

    const int N = fileList.size();
    int newIndex = qBound(0, i, N - 1);
    if (newIndex == index) return index; //相同不加载
    QString filePath = curDirPath + '/' + fileList[index = newIndex];
    setPixmap(filePath);
    return index;
}

int Widget::switchPixmap(Widget::SwitchPix dir)
{
    return switchPixmap(dir == PRE ? index - 1 : index + 1);
}

void Widget::rotateClockwise()
{
    if (isGif) return;

    QTransform trans = QTransform().rotate(90); //旋转图像
    pixmap = pixmap.transformed(trans, Qt::SmoothTransformation);
    toShow = getScaledPixmap(pixmap, scaleSize, isShadowDrop, Qt::SmoothTransformation);

    QSize size = pixRect.size();
    QPoint center = pixRect.center(); //旋转逻辑框
    pixRect.setSize(QSize(size.height(), size.width()));
    pixRect.moveCenter(center);

    updateAll();
    updateThumbnailPixmap();
}

void Widget::copyToClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setPixmap(getScaledPixmap(pixmap, scaleSize, false, Qt::SmoothTransformation)); //防止图片过大 同时也可以手动调整大小

    // setImageData可以，setData不可以 可能是缺少什么数据
    // mimeData->setImageData(this->pixmap);
    // QApplication::clipboard()->setMimeData(mimeData);

    showTip("Copied to Clipboard");
}

void Widget::showTip(const QString& tip, int time)
{
    constexpr int H = 45;

    ui->label_tip->setText(tip);
    ui->label_tip->adjustSize();
    ui->label_tip->move(width()/2 - ui->label_tip->width()/2, H);
    ui->label_tip->show();

    // QTimer::singleShot不好重置时间
    static QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(time);
    timer->callOnTimeout([=]() { ui->label_tip->hide(); });

    timer->stop(); // 重置时间
    timer->start();
}

bool Widget::isTopMode()
{
    return ui->btn_pin->isChecked();
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    curPos = event->globalPos();
    if (!isOnPixmap(curPos)) return; //防止鼠标在其他部分移动
    if (event->button() == Qt::LeftButton)
        canMovePix = true;
    else if (event->button() == Qt::RightButton) { //延迟触发：长按为Menu 短按为quit
        QTimer::singleShot(MenuDelay, this, &Widget::menuRequested);
    }
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    if (!isOnPixmap(curPos)) return; //防止鼠标在其他部分移动
    auto button = event->button();

    if (button == Qt::RightButton) {
        if (isMenuRequested) //延迟触发：长按为Menu 短按为quit
            emit menuCloseRequested();
        else
            qApp->quit();
    } else if (button == Qt::LeftButton)
        canMovePix = false;
    else if (button == Qt::BackButton) //鼠标侧键
        switchPixmap(NEXT);
    else if (button == Qt::ForwardButton) //鼠标侧键
        switchPixmap(PRE);
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

void Widget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        switchPixmap(PRE);
        break;
    case Qt::Key_Right:
        switchPixmap(NEXT);
        break;
    default:
        break;
    }
}

void Widget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return; //过滤自动触发

    switch (event->key()) {
    case Qt::Key_Space: //100%
        scaleAndMove(1.0, this->rect().center());
        break;
    case Qt::Key_Backspace: //适应屏幕
        scaleAndMove(scaleToScreen(pixmap), this->rect().center());
        break;
    case Qt::Key_Escape:
        qApp->quit();
        break;
    case Qt::Key_C: //复制到剪切板
        if (event->modifiers() & Qt::ControlModifier)
            copyToClipboard();
        break;
    default:
        break;
    }
}

void Widget::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) { //Ctrl+滚轮切换图片
        switchPixmap(event->angleDelta().y() > 0 ? PRE : NEXT);
    } else {
        qreal scale = event->angleDelta().y() > 0 ? scaleSize * 1.1 : scaleSize / 1.1; //观察微软原生Photo得出结论
        scalePixmap(scale, event->globalPosition().toPoint());
    }
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
    if (qApp->focusWidget() && qApp->focusWidget()->window() == this)
        return; // 焦点在子窗口（如QMenu），无需隐藏

    ui->label_info->hide();
    ui->Btns->hide();
}
