#include "appconnswindow.h"

#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>

#include <conf/app.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <model/appconnsearchmodel.h>
#include <user/iniuser.h>

using namespace Fort;

namespace {

/* A new program has no name yet */
QString appNameOrPath(const App &app)
{
    return app.appName.isEmpty() ? app.appPath : app.appName;
}

}

AppConnsWindow::AppConnsWindow(const App &app, QWidget *parent) :
    AppConnsWindow(app, parent, Qt::Window)
{
    initialize();

    setupFormWindow(iniUser(), IniUser::connWindowGroup());
}

AppConnsWindow::AppConnsWindow(const App &app, QWidget *parent, Qt::WindowFlags f) :
    AppConnsWindow(new AppConnSearchModel(), app, parent, f)
{
}

AppConnsWindow::AppConnsWindow(
        AppConnSearchModel *connListModel, const App &app, QWidget *parent, Qt::WindowFlags f) :
    ConnectionsWindow(connListModel, parent, f)
{
    setupUi();

    updateApp(app);
}

AppConnSearchModel *AppConnsWindow::appConnListModel() const
{
    return static_cast<AppConnSearchModel *>(connListModel());
}

void AppConnsWindow::setApp(const App &app)
{
    const bool isFilterChanged = updateApp(app);

    updateLogConnIcons();

    // The program's edits keep the listed connections
    if (isFilterChanged) {
        connListModel()->updateSearchLater();
    }
}

void AppConnsWindow::setAddFilterVisible(bool visible)
{
    addFilterAction()->setVisible(visible);
}

void AppConnsWindow::restoreWindowState()
{
    /* The Program Connections window doesn't keep its state */
    auto owner = parentWidget();
    if (!owner) {
        resize(1024, 768);
        return;
    }

    resize(800, minimumHeight());
    moveNextTo(owner);
}

void AppConnsWindow::retranslateWindowTitle()
{
    this->setWindowTitle(tr("Program Connections"));
}

void AppConnsWindow::setupUi()
{
    // The program's name instead of the editing
    hideEditing();

    // Program Name
    setupEditAppName();

    m_appNameSeparator = ControlUtil::createVSeparator();

    auto layout = headerLayout();
    layout->insertWidget(0, m_editAppName);
    layout->insertWidget(1, m_appNameSeparator);

    // Add Filter
    setupAddFilter();

    // Size
    this->setMinimumHeight(300);
}

void AppConnsWindow::setupEditAppName()
{
    m_editAppName = new LineEdit();
    m_editAppName->setReadOnly(true);
    m_editAppName->setMinimumWidth(100);
    m_editAppName->setMaximumWidth(300);
}

void AppConnsWindow::setupAddFilter()
{
    addFilterAction()->setVisible(false); // by setAddFilterVisible()

    // Also by the double click, when it's visible
    connect(connListView(), &TableView::doubleClicked, this, [&] {
        if (addFilterAction()->isVisible()) {
            addFilterAction()->trigger();
        }
    });
}

void AppConnsWindow::addConnFilter(const ConnRow &connRow)
{
    emit addFilterRequested(connRow);
}

bool AppConnsWindow::updateApp(const App &app)
{
    const bool isFilterChanged = appConnListModel()->setApp(app);

    m_editAppName->setStartText(appNameOrPath(app));

    m_appLogAllowedConn = app.logAllowedConn;
    m_appLogBlockedConn = app.logBlockedConn;

    updateProgramColumnHidden();

    return isFilterChanged;
}

void AppConnsWindow::hideAppName()
{
    m_editAppName->hide();
    m_appNameSeparator->hide();
}

void AppConnsWindow::setupAlertWindow()
{
    initialize();

    setupFormWindow(iniUser(), IniUser::connWindowGroup());

    // Save the state on close
    connect(this, &WidgetWindow::aboutToClose, this, [&] { saveWindowState(/*wasVisible=*/true); });
}

void AppConnsWindow::restoreAlertWindowState()
{
    // Next to the alert window, if there is no saved geometry
    AppConnsWindow::restoreWindowState();

    restoreIniWindowGeometry(size());
    restoreIniConnListHeader();

    // By the program's path
    updateProgramColumnHidden();
}

void AppConnsWindow::updateProgramColumnHidden()
{
    auto header = connListView()->horizontalHeader();

    // A wildcard program's connections have different paths
    const bool isAppPath = !appConnListModel()->appPath().isEmpty();
    header->setSectionHidden(int(ConnListColumn::Program), /*hide=*/isAppPath);
}

bool AppConnsWindow::logAllowedConn() const
{
    return ConnectionsWindow::logAllowedConn() && m_appLogAllowedConn;
}

bool AppConnsWindow::logBlockedConn() const
{
    return ConnectionsWindow::logBlockedConn() && m_appLogBlockedConn;
}
