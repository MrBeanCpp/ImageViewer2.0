#include "widget.h"
#include <QApplication>
#include <QDebug>
#include <Shlobj.h>
#include <shlwapi.h>
void assocWithExe(QString appPath, QString className, const QStringList& exts, QString extDes = QString());
int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QString appPath = QDir::toNativeSeparators(qApp->applicationFilePath());
    QStringList exts; //.png
    for (auto& str : Widget::Filter) //*.png
        exts << ('.' + QFileInfo(str).suffix());

    assocWithExe(appPath, "ImageViewer_2File", exts);

    Widget w;
    w.show();
    return a.exec();
}
void assocWithExe(QString appPath, QString className, const QStringList& exts, QString extDes)
{
    Q_UNUSED(extDes)
    QString baseUrl("HKEY_CURRENT_USER\\Software\\Classes"); // 要添加的顶层目录
    QSettings settingClasses(baseUrl, QSettings::NativeFormat);

    if (settingClasses.contains("/" + className + "/DefaultIcon/.")) {
        qDebug() << "键值已存在";
        return;
    }

    settingClasses.setValue("/" + className + "/Shell/Open/Command/.", "\"" + appPath + "\" \"%1\"");
    settingClasses.setValue("/" + className + "/DefaultIcon/.", appPath + ",0");
    //settingClasses.setValue("/" + className + "/.", extDes);

    for (auto& ext : exts)
        settingClasses.setValue("/" + ext + "/OpenWithProgIds/" + className, "");

    settingClasses.sync(); // 立即保存该修改
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0); //通知系统更新
}
