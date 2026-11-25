#include "taskbargraphwindow.h"

#include <QTimer>

#include <form/graph/taskbarhost.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>

using namespace Fort;

TaskbarGraphWindow::TaskbarGraphWindow(int graphId, QWidget *parent) :
    GraphBaseWindow(graphId, parent)
{
    setupUi();

    setupFormWindow(iniUser());

    setupTaskbarHost();
}

TaskbarGraphWindow::~TaskbarGraphWindow()
{
    windowManager()->taskbarHost()->removeWindow(this);
}

bool TaskbarGraphWindow::isHostChanged() const
{
    if (GraphBaseWindow::isHostChanged())
        return true;

    // The taskbar's mode is changed while the window is hidden
    const bool layered = testAttribute(Qt::WA_TranslucentBackground);

    return layered != windowManager()->taskbarHost()->canLayerWindows();
}

void TaskbarGraphWindow::setupUi()
{
    // The speed is left of the graph: the taskbar's height is small
    plot()->setSpeedLeft(true);

    // The width
    connect(view(), &GraphView::optionsUpdated, this, &TaskbarGraphWindow::updatePlacement);

    // The background's alpha per pixel: Windows 11 shows only a layered window in its taskbar
    if (windowManager()->taskbarHost()->canLayerWindows()) {
        setAttribute(Qt::WA_TranslucentBackground);
    }

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
            | Qt::WindowDoesNotAcceptFocus);
}

void TaskbarGraphWindow::setupTaskbarHost()
{
    connect(windowManager()->taskbarHost(), &TaskbarHost::windowsInvalidated, this, [this] {
        if (isHidden())
            return; // not placed: checked on its showing

        recreate();
    });
}

void TaskbarGraphWindow::recreate()
{
    const int graphId = this->graphId();

    QTimer::singleShot(0, windowManager(), [=] { windowManager()->recreateGraphWindow(graphId); });
}

void TaskbarGraphWindow::showEvent(QShowEvent *event)
{
    GraphBaseWindow::showEvent(event);

    updatePlacement();

    // The native show after the event applies the widget's own geometry
    QMetaObject::invokeMethod(this, &TaskbarGraphWindow::updatePlacement, Qt::QueuedConnection);
}

void TaskbarGraphWindow::hideEvent(QHideEvent *event)
{
    GraphBaseWindow::hideEvent(event);

    windowManager()->taskbarHost()->removeWindow(this);
}

void TaskbarGraphWindow::updatePlacement()
{
    if (!isVisible())
        return;

    TaskbarHost::WindowOptions options;
    options.order = graphId();
    options.length = iniGraph().taskbarWidth();

    windowManager()->taskbarHost()->addWindow(this, options);
}
