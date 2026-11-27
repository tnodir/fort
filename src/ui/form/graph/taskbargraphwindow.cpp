#include "taskbargraphwindow.h"

#include <QLayout>
#include <QTimer>

#include <algorithm>
#include <array>

#include <qt_windows.h>

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

QList<GraphSide> TaskbarGraphWindow::popupSides(const QRect &rect, const QRect &screenRect) const
{
    // By the GraphSide
    const std::array<int, GraphSideCount> edgeDistances = {
        rect.top() - screenRect.top(),
        screenRect.bottom() - rect.bottom(),
        rect.left() - screenRect.left(),
        screenRect.right() - rect.right(),
    };

    // Away from the screen's edge nearest to the graph: of the taskbar
    const auto nearestEdge = std::min_element(edgeDistances.begin(), edgeDistances.end());
    const auto side = GraphSide(nearestEdge - edgeDistances.begin());

    return { GraphTypes::oppositeSide(side) };
}

void TaskbarGraphWindow::setupUi()
{
    // The size is of the taskbar: the layout's minimum (of the view's hidden scroll bars) may be
    // taller, and QWidget::setGeometry() would raise the height to it
    layout()->setSizeConstraint(QLayout::SetNoConstraint);

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

bool TaskbarGraphWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    // Qt makes a window not accepting the focus the focus window on WM_SETFOCUS, then on another
    // window's WM_SETFOCUS within SetParent() re-activates it with a warning
    // (QWindowsContext::handleFocusEvent() of Qt 6.1.3 on Windows 7)
    const auto msg = static_cast<MSG *>(message);
    if (msg->message == WM_SETFOCUS)
        return true;

    return GraphBaseWindow::nativeEvent(eventType, message, result);
}
