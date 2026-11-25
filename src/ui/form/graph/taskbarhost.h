#ifndef TASKBARHOST_H
#define TASKBARHOST_H

#include <QRect>

class QWidget;

// Places a window over the primary monitor's taskbar, left of its notification area
class TaskbarHost
{
public:
    // Puts the window topmost over the taskbar; the width is in physical pixels
    static bool placeOverlay(QWidget *window, int width);

    // The taskbar's free area for the window, in physical pixels
    static QRect overlayRect(int width);
};

#endif // TASKBARHOST_H
