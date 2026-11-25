#include "taskbarhost.h"

#include <QWidget>

#include <qt_windows.h>

namespace {

bool getWindowRect(HWND hwnd, QRect &rect)
{
    RECT r;
    if (!hwnd || !GetWindowRect(hwnd, &r))
        return false;

    rect = QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
    return true;
}

}

bool TaskbarHost::placeOverlay(QWidget *window, int width)
{
    const QRect rect = overlayRect(width);
    if (rect.isEmpty())
        return false;

    return SetWindowPos((HWND) window->winId(), HWND_TOPMOST, rect.x(), rect.y(), rect.width(),
            rect.height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

QRect TaskbarHost::overlayRect(int width)
{
    const HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);

    QRect trayRect;
    if (!getWindowRect(tray, trayRect))
        return {};

    // The notification area's left edge, else the taskbar's right one
    int right = trayRect.right() + 1;

    QRect notifyRect;
    if (getWindowRect(FindWindowExW(tray, nullptr, L"TrayNotifyWnd", nullptr), notifyRect)) {
        right = notifyRect.left();
    }

    return QRect(right - width, trayRect.top(), width, trayRect.height());
}
