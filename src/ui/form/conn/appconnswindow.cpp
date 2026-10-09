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

inline constexpr QSize APP_CONNS_WINDOW_SIZE(1024, 768);

inline constexpr int OWNED_WINDOW_WIDTH = 800;
inline constexpr int OWNED_WINDOW_SPACING = 10;

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
    ConnectionsWindow(new AppConnSearchModel(), parent, f)
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
        resize(APP_CONNS_WINDOW_SIZE);
        return;
    }

    // Next to the owner window, not over it: the owned window stays above it
    resize(OWNED_WINDOW_WIDTH, minimumHeight());
    moveNextTo(owner, OWNED_WINDOW_SPACING);
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

    m_actAddFilter = new QAction(IconCache::icon(":/icons/filter.png"), QString(), menu);
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

void AppConnsWindow::updateProgramColumnHidden()
{
    auto header = connListView()->horizontalHeader();

    // A wildcard program's connections have different paths
    const bool isAppPath = !appConnListModel()->appPath().isEmpty();
    header->setSectionHidden(int(ConnListColumn::Program), /*hide=*/isAppPath);
}
