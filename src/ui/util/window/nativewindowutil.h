#ifndef NATIVEWINDOWUTIL_H
#define NATIVEWINDOWUTIL_H

#include <QRect>
#include <qwindowdefs.h>

QT_FORWARD_DECLARE_CLASS(QWidget)

// The windows' native handles and geometry, in the device pixels
class NativeWindowUtil
{
public:
    static HWND toHwnd(WId wid) { return reinterpret_cast<HWND>(wid); }
    static HWND windowHwnd(QWidget *window);

    // In the parent's client coordinates, else in the screen's
    static QRect windowRect(HWND hwnd, HWND parent = nullptr);
    static QRect clientRect(HWND hwnd);

    // The widget's geometry follows the native one at once (exact in the device-independent
    // pixels): else the native show after its show event applies the widget's own geometry
    static void syncWidgetGeometry(QWidget *window);
};

#endif // NATIVEWINDOWUTIL_H
