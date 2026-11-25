#include "formpointer.h"

#include <form/activity/activitywindow.h>
#include <form/conn/connectionswindow.h>
#include <form/filtersim/filtersimwindow.h>
#include <form/graph/graphswindow.h>
#include <form/graph/graphwindow.h>
#include <form/group/groupswindow.h>
#include <form/install/installationwindow.h>
#include <form/main/mainwindow.h>
#include <form/main/paneldock.h>
#include <form/opt/optionswindow.h>
#include <form/prog/programalertwindow.h>
#include <form/prog/programswindow.h>
#include <form/rule/ruleswindow.h>
#include <form/speedlimit/speedlimitswindow.h>
#include <form/svc/serviceswindow.h>
#include <form/timeperiod/timeperiodswindow.h>
#include <form/traf/trafficwindow.h>
#include <form/update/updateswindow.h>
#include <form/zone/zoneswindow.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/bitutil.h>
#include <util/window/widgetwindowstatewatcher.h>

using namespace Fort;

namespace {

template<class T>
FormWindow *createWindow(int /*id*/)
{
    return new T();
}

FormWindow *createGraphWindow(int id)
{
    return new GraphWindow(id);
}

using createWindow_func = FormWindow *(*)(int id);

static const createWindow_func createWindow_funcList[] = {
    &createWindow<MainWindow>,
    &createWindow<ProgramsWindow>,
    &createWindow<ProgramAlertWindow>,
    &createWindow<ServicesWindow>,
    &createWindow<OptionsWindow>,
    &createWindow<RulesWindow>,
    &createWindow<TrafficWindow>,
    &createWindow<ActivityWindow>,
    &createWindow<ConnectionsWindow>,
    &createWindow<ZonesWindow>,
    &createWindow<GroupsWindow>,
    &createWindow<SpeedLimitsWindow>,
    &createWindow<TimePeriodsWindow>,
    &createWindow<FilterSimWindow>,
    &createGraphWindow,
    &createWindow<InstallationWindow>,
    &createWindow<UpdatesWindow>,
    &createWindow<GraphsWindow>,
};

static_assert(sizeof(createWindow_funcList) / sizeof(createWindow_funcList[0]) == WindowCount,
        "Missing a WindowCode in createWindow_funcList[]");

FormWindow *createWindowByCode(WindowCode code, int id)
{
    const int index = BitUtil::bitScanForward(code);

    Q_ASSERT(index >= 0 && index < WindowCount);

    const createWindow_func func = createWindow_funcList[index];

    return func(id);
}

}

FormPointer::FormPointer(WindowCode code, qint8 id) : m_id(id), m_code(code) { }

FormWindow *FormPointer::initialize()
{
    Q_ASSERT(!m_window);

    auto w = createWindowByCode(code(), id());
    m_window.reset(w);

    QObject::connect(w, &FormWindow::aboutToClose, [&] { close(); });

    return w;
}

bool FormPointer::show(bool activate, bool expose)
{
    auto windowManager = Fort::windowManager();

    if (!windowManager->checkWindowPassword(code()))
        return false;

    if (!window()) {
        initialize();
    }

    if (!showWindow(activate, expose))
        return false;

    windowManager->windowOpened(code(), id());

    return true;
}

bool FormPointer::restoreDocked()
{
    if (!window()) {
        initialize();
    }

    if (!isIniDocked())
        return false;

    if (!showDocked(/*activate=*/false, /*expose=*/false))
        return false;

    Fort::windowManager()->windowOpened(code(), id());

    return true;
}

bool FormPointer::close(bool isAppQuitting)
{
    auto w = this->window();
    if (!w) {
        return false;
    }

    auto windowManager = Fort::windowManager();
    isAppQuitting = isAppQuitting || windowManager->isAppQuitting();

    if (isShown()) {
        w->saveWindowState(isAppQuitting);

        takeFromDock();

        w->hide();

        w->onWindowClosed();

        windowManager->windowClosed(code(), id());
    }

    if (isAppQuitting || w->deleteOnClose()) {
        emit w->aboutToDelete();
        w->deleteLater();

        m_window.release(); // will be deleted later
        return true;
    }

    return false;
}

PanelDock *FormPointer::dock() const
{
    return m_dock;
}

bool FormPointer::isShown() const
{
    // A docked window may be hidden in its tab
    return m_window && (m_window->isVisible() || m_dock);
}

bool FormPointer::moveToDock()
{
    auto w = this->window();
    if (!w || m_dock)
        return false;

    if (!w->isDockable())
        return false;

    setIniDocked(true);

    w->saveWindowState(/*wasVisible=*/true);

    // The re-parenting clears it: the Main window's focus child, maybe in a hidden tab, gets it
    const QPointer<QWidget> focus = w->focusWidget();

    if (!showDocked(/*activate=*/true, /*expose=*/false)) {
        setIniDocked(false);
        return false;
    }

    if (focus) {
        focus->setFocus();
    }

    return true;
}

bool FormPointer::moveToWindow(const std::optional<QPoint> &pos)
{
    auto w = this->window();
    if (!w || !m_dock)
        return false;

    setIniDocked(false);

    w->saveWindowState(/*wasVisible=*/true);

    takeFromDock();

    w->restoreWindowState();

    auto windowManager = Fort::windowManager();

    if (pos) {
        w->move(*pos);
    } else if (w->stateWatcher()->geometry().isNull()) {
        // Never shown as a window: centered on the Main window
        w->centerTo(windowManager->mainWindow());
    }

    // A new native window of the undocked one
    if (w->excludeFromCapture()) {
        WidgetWindow::excludeWindowFromCapture(w);
    }

    w->showWindow();

    windowManager->taskbarButton().setWindowBadge(w->windowHandle());

    return true;
}

bool FormPointer::isIniDocked() const
{
    const auto w = this->window();

    return w->isDockable() && iniUser().windowDocked(w->iniGroup());
}

void FormPointer::setIniDocked(bool docked)
{
    iniUser().setWindowDocked(window()->iniGroup(), docked);
}

bool FormPointer::showWindow(bool activate, bool expose)
{
    if (isIniDocked()) {
        return showDocked(activate, expose);
    }

    window()->showWindow(activate, expose);

    return true;
}

bool FormPointer::showDocked(bool activate, bool expose)
{
    auto windowManager = Fort::windowManager();

    // The Main window may dock this window, restoring its layout
    if (!windowManager->showWindowByCode(WindowMain, activate, expose))
        return false;

    if (!m_dock) {
        m_dock = windowManager->mainWindow()->addDock(window());
    }

    m_dock->show();
    m_dock->raise();

    window()->showWindow(activate);

    return true;
}

void FormPointer::takeFromDock()
{
    if (!m_dock)
        return;

    Fort::windowManager()->mainWindow()->removeDock(m_dock);

    m_dock = nullptr;
}
