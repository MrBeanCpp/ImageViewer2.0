#include "winEventHook.h"
#include <QDebug>
#include <QTime>

static QList<HWINEVENTHOOK> handlers;
static WinEventCallback callback = nullptr;

void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (idObject != OBJID_WINDOW)
        return;

    if (callback)
        callback(event, hwnd);
}

bool setWinEventHook(WinEventCallback callback)
{
    ::callback = callback;

    // WINEVENT_OUTOFCONTEXT：表示回调函数是在调用线程的上下文中调用的，而不是在生成事件的线程的上下文中。这种方式不需要DLL模块句柄（hmodWinEventProc 设置为 NULL）
    handlers << SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    handlers << SetWinEventHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    handlers << SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    handlers << SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE , EVENT_OBJECT_LOCATIONCHANGE , NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);

    qDebug() << "Set WinEventHook.";

    return handlers.indexOf(nullptr) == -1; // All not nullptr
}

void unhookWinEvent()
{
    if (handlers.count(nullptr) == handlers.size()) // All nullptr
        return;

    qDebug() << "Unhook win event.";

    for (auto& h : handlers) {
        if (h) {
            UnhookWinEvent(h);
            h = nullptr;
        }
    }
}
