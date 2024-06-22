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

// 删除注册表项
void registry_delete(HKEY mainPath, const QString& subPath,const QString& key)
{
    HKEY hKey = NULL;
    if (RegOpenKeyEx(mainPath, subPath.toStdWString().c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValue(hKey, key.toStdWString().c_str());
        RegCloseKey(hKey);
    }
}

// 清除exe描述信息缓存，否则exe信息更新之后，只要文件名（路径）不变，Windows就不会更新它（哼）;
// WARNING: 如果key是一个路径（包含'\' or '/'），则QSettings无法处理 只能采用 Windows API
// `QSettings 始终将反斜杠视为特殊字符，并且不提供用于读取或写入此类条目的 API` - https://runebook.dev/zh/docs/qt/qsettings
void clearMuiCache(const QString& exePath) //清除mui缓存
{
    const QString MuiCache = R"(Software\Classes\Local Settings\Software\Microsoft\Windows\Shell\MuiCache)";
    // 删除之后，会触发系统更新缓存，自动重新读取exe信息（描述）
    registry_delete(HKEY_CURRENT_USER, MuiCache, exePath + ".FriendlyAppName"); // 右键png图片-打开方式：显示的名称
    registry_delete(HKEY_CURRENT_USER, MuiCache, exePath + ".ApplicationCompany");

    qDebug() << "Registry - MuiCache cleared";
}

void clearOpenList(const QString& exePath, const QStringList& exts) //清除最近打开列表 否则更改路径后可能出现两个ImageViewer
{
    const QString baseUrl = R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\)";
    QSettings openList(baseUrl, QSettings::NativeFormat);
    for (auto& ext : exts) {
        openList.beginGroup(ext + "/OpenWithList");
        const QStringList keys = openList.allKeys();
        for (auto& key: keys) {
            if (key == "MRUList") continue; // 排序用的 貌似
            auto filename = openList.value(key).toString();
            if (exePath.contains(filename)) {
                openList.remove(key);
                qDebug() << "removed:" << ext << key << " - " << filename;
            }
        }
        openList.endGroup();
    }
}

void assocWithExe(QString appPath, QString className, const QStringList& exts, QString extDes)
{ //效果是再次打开该后缀的文件时，会弹出提示，询问用户要如何打开文件（并提示本程序为新增程序）
    Q_UNUSED(extDes)
    QString baseUrl("HKEY_CURRENT_USER\\Software\\Classes"); // 要添加的顶层目录
    QSettings settingClasses(baseUrl, QSettings::NativeFormat);

    // 前缀'/'感觉没啥用 应该可以去掉
    const QString defaultIcon = "/" + className + "/DefaultIcon/."; // 默认图标路径
    const QString shellOpenCommand = "/" + className + "/Shell/Open/Command/."; // 打开命令

    QString command = settingClasses.value(shellOpenCommand).toString();

    if (command.contains(appPath)) { // 路径 or name变化后要重新关联
        qDebug() << "Registry - 文件已关联 无需操作";
        return;
    }

    clearMuiCache(appPath); // 清除mui缓存 其实最好是每次都执行，但是为了性能取舍
    clearOpenList(command, exts); // 清除最近打开列表 中 旧的（文件名 or 路径）ImageViewer

    settingClasses.setValue(shellOpenCommand, QString(R"("%1" "%2")").arg(appPath, "%1"));
    settingClasses.setValue(defaultIcon, appPath + ",0");
    //settingClasses.setValue("/" + className + "/.", extDes);

    for (auto& ext : exts)
        settingClasses.setValue("/" + ext + "/OpenWithProgIds/" + className, "");

    qDebug() << "Registry - 文件关联成功";
    settingClasses.sync(); // 立即保存该修改
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0); //通知系统更新
}
