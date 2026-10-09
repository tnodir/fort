#include "appconnswindow.h"

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

inline constexpr QSize APP_CONNS_WINDOW_SIZE(1024, 768);

/* A new program has no name yet */
QString appNameOrPath(const App &app)
{
    return app.appName.isEmpty() ? app.appPath : app.appName;
}

}

AppConnsWindow::AppConnsWindow(const App &app, QWidget *parent) :
    ConnectionsWindow(new AppConnSearchModel(), parent, Qt::Window)
{
    setupUi();

    updateApp(app);

    initialize();

    setupFormWindow(iniUser(), IniUser::connWindowGroup());
}

AppConnSearchModel *AppConnsWindow::appConnListModel() const
{
    return static_cast<AppConnSearchModel *>(connListModel());
}

void AppConnsWindow::restoreWindowState()
{
    /* The Program Connections window doesn't keep its state */
    resize(APP_CONNS_WINDOW_SIZE);
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

    auto layout = headerLayout();
    layout->insertWidget(0, m_editAppName);
    layout->insertWidget(1, ControlUtil::createVSeparator());
}

void AppConnsWindow::setupEditAppName()
{
    m_editAppName = new LineEdit();
    m_editAppName->setReadOnly(true);
    m_editAppName->setMinimumWidth(100);
    m_editAppName->setMaximumWidth(300);
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
