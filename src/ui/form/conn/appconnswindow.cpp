#include "appconnswindow.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>

#include <conf/app.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <model/appconnsearchmodel.h>
#include <user/iniuser.h>
#include <util/iconcache.h>

#include "connectionscontroller.h"

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
    setupController();

    updateApp(app);
}

AppConnSearchModel *AppConnsWindow::appConnListModel() const
{
    return static_cast<AppConnSearchModel *>(connListModel());
}

void AppConnsWindow::setApp(const App &app)
{
    updateApp(app);

    connListModel()->updateSearchLater();
}

void AppConnsWindow::setAddFilterVisible(bool visible)
{
    m_actAddFilter->setVisible(visible);
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

void AppConnsWindow::setupController()
{
    connect(ctrl(), &ConnectionsController::retranslateUi, this, &AppConnsWindow::retranslateUi);
}

void AppConnsWindow::retranslateUi()
{
    m_actAddFilter->setText(tr("Add Filter"));
}

void AppConnsWindow::setupUi()
{
    // The program's name instead of the editing
    hideEditing();

    // Program Name
    setupEditAppName();

    auto layout = headerLayout();
    layout->insertWidget(0, m_editAppName);
    layout->insertWidget(1, ControlUtil::createVSeparator());

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
    auto menu = connListView()->menu();
    const auto firstAction = menu->actions().value(0);

    m_actAddFilter = new QAction(IconCache::icon(":/icons/filter_add.png"), QString(), menu);
    m_actAddFilter->setVisible(false); // by setAddFilterVisible()

    menu->insertAction(firstAction, m_actAddFilter);
    menu->insertSeparator(firstAction);

    connect(m_actAddFilter, &QAction::triggered, this,
            [&] { emit addFilterRequested(currentConnRow()); });

    const auto refreshAddFilter = [&] { m_actAddFilter->setEnabled(connListCurrentIndex() >= 0); };

    refreshAddFilter();

    connect(connListView(), &TableView::currentIndexChanged, this, refreshAddFilter);
}

void AppConnsWindow::updateApp(const App &app)
{
    appConnListModel()->setApp(app);

    m_editAppName->setStartText(appNameOrPath(app));

    updateProgramColumnHidden();
}

void AppConnsWindow::setupAlertWindow()
{
    initialize();

    setupFormWindow(iniUser(), IniUser::connWindowGroup());

    // Save the state on close
    connect(this, &WidgetWindow::aboutToClose, this, [&] { saveWindowState(/*wasVisible=*/true); });
}

void AppConnsWindow::restoreAlertWindowState(const QString &iniGroup)
{
    // Next to the alert window, if there is no saved geometry
    AppConnsWindow::restoreWindowState();

    restoreIniWindowState(iniGroup, size());

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
