#include "nativewindowutil.h"

#include <QWidget>
#include <QWindow>

#include <qt_windows.h>

namespace {

QRect toQRect(const RECT &r)
{
    return QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

}

HWND NativeWindowUtil::windowHwnd(QWidget *window)
{
    return toHwnd(window->winId());
}

QRect NativeWindowUtil::windowRect(HWND hwnd, HWND parent)
{
    RECT r;
    if (!hwnd || !GetWindowRect(hwnd, &r))
        return {};

    MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<POINT *>(&r), 2);

    return toQRect(r);
}

QRect NativeWindowUtil::clientRect(HWND hwnd)
{
    RECT r;
    if (!GetClientRect(hwnd, &r))
        return {};

    return toQRect(r);
}

NativeWindowUtil::MonitorRects NativeWindowUtil::monitorRects(HWND hwnd)
{
    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi))
        return {};

    return { .screen = toQRect(mi.rcMonitor), .work = toQRect(mi.rcWork) };
}

void NativeWindowUtil::syncWidgetGeometry(QWidget *window)
{
    window->setGeometry(window->windowHandle()->geometry());
}
