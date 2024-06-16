#include "util.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <psapi.h>
#include <system_error>
#include <shobjidl.h>
#include <propkey.h>
#include <comdef.h>
#include <atlbase.h>

bool Util::setWindowTopMost(QWidget* widget, bool top)
{
    if (widget == nullptr)
        return false;

    return SetWindowPos(HWND(widget->winId()), top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

bool Util::setWindowTop(HWND hwnd)
{
    return SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

QString Util::getDirPath(const QString& filePath)
{
    return QFileInfo(filePath).absoluteDir().absolutePath();
}

QString Util::getFileName(const QString& filePath)
{
    return QFileInfo(filePath).fileName();
}

QString Util::getWindowText(HWND hwnd)
{
    if (hwnd == nullptr) return QString();

    static WCHAR text[128];
    GetWindowTextW(hwnd, text, _countof(text)); //sizeof(text)字节数256 内存溢出
    return QString::fromWCharArray(text);
}

QString Util::getProcessExePath(HWND hwnd)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    if (processId == 0)
        return "";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
        return "";

    TCHAR processName[MAX_PATH] = TEXT("<unknown>");
    // https://www.cnblogs.com/mooooonlight/p/14491399.html
    if (GetModuleFileNameEx(hProcess, NULL, processName, MAX_PATH)) {
        CloseHandle(hProcess);
        return QString::fromWCharArray(processName);
    }

    CloseHandle(hProcess);
    return "";
}

QString Util::getFileDescription(const QString& path)
{
    CoInitialize(nullptr); // 初始化 COM 库

    std::wstring wStr = path.toStdWString();
    LPCWSTR pPath = wStr.c_str();

    // 使用 CComPtr 自动释放 IShellItem2 接口
    CComPtr<IShellItem2> pItem;
    HRESULT hr = SHCreateItemFromParsingName(pPath, nullptr, IID_PPV_ARGS(&pItem));
    if (FAILED(hr))
        throw std::system_error(hr, std::system_category(), "SHCreateItemFromParsingName() failed");

    // 使用 CComHeapPtr 自动释放字符串（调用 CoTaskMemFree）
    CComHeapPtr<WCHAR> pValue;
    hr = pItem->GetString(PKEY_FileDescription, &pValue);
    if (FAILED(hr))
        throw std::system_error(hr, std::system_category(), "IShellItem2::GetString() failed");

    CoUninitialize(); // 取消初始化 COM 库
    return QString::fromWCharArray(pValue);
}

QPoint Util::getWindowPos(HWND hwnd)
{
    RECT currentRect;
    GetWindowRect(hwnd, &currentRect);
    return QPoint(currentRect.left, currentRect.top);
}

QString Util::getProcessDescription(HWND hwnd)
{
    QString exePath = getProcessExePath(hwnd);
    return getFileDescription(exePath);
}
