#include "taskbargraphwindow.h"

#include <conf/confmanager.h>
#include <form/graph/taskbarhost.h>
#include <fortglobal.h>
#include <user/iniuser.h>

using namespace Fort;

TaskbarGraphWindow::TaskbarGraphWindow(int graphId, QWidget *parent) :
    GraphBaseWindow(graphId, parent)
{
    setupUi();

    setupFormWindow(iniUser());

    connect(confManager(), &ConfManager::iniUserChanged, this,
            &TaskbarGraphWindow::updatePlacement);
}

void TaskbarGraphWindow::setupUi()
{
    // The speed is left of the graph: the taskbar's height is small
    plot()->setSpeedLeft(true);

    // The taskbar's layout may change: the placement is kept up to date by the graph's updates
    connect(view(), &GraphView::speedUpdated, this, &TaskbarGraphWindow::updatePlacement);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
            | Qt::WindowDoesNotAcceptFocus);
}

void TaskbarGraphWindow::showEvent(QShowEvent *event)
{
    GraphBaseWindow::showEvent(event);

    updatePlacement();

    // The native show after the event applies the widget's own geometry
    QMetaObject::invokeMethod(this, &TaskbarGraphWindow::updatePlacement, Qt::QueuedConnection);
}

void TaskbarGraphWindow::updatePlacement()
{
    if (!isVisible())
        return;

    const int width = qRound(iniGraph().taskbarWidth() * devicePixelRatioF());

    TaskbarHost::placeOverlay(this, width);
}
