QT       += core gui
QT += winextras
QT += concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    circlemenu.cpp \
    main.cpp \
    util.cpp \
    widget.cpp \
    winEventHook.cpp \
    winthumbnailtoolbar.cpp

HEADERS += \
    circlemenu.h \
    util.h \
    widget.h \
    winEventHook.h \
    winthumbnailtoolbar.h

FORMS += \
    circlemenu.ui \
    widget.ui

LIBS += -lDwmapi -lGdi32 -lShlwapi -luser32

RC_ICONS = images/ICON.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    log.txt

RESOURCES += \
    res.qrc

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}

TARGET = "ImageViewer 2.0"

# 会自动在build目录下生成.rc文件并链接
# 版本
VERSION = 1.5.2
# 公司名称
QMAKE_TARGET_COMPANY = "MrBeanC"
# 文件说明
QMAKE_TARGET_DESCRIPTION = "ImageViewer 2.0"


# 宏定义 for C++ code；这里修改之后需要重新编译 或 修改用到宏的文件以触发重新编译 否则不会生效
# DEFINES += APP_VERSION=\\\"$$VERSION\\\"
# No, Just use qApp->applicationVersion() while updates automatically
