#include "optionswindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/osutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "optionscontroller.h"
#include "pages/optmainpage.h"

using namespace Fort;

OptionsWindow::OptionsWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new OptionsController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::optWindowGroup());

    connect(this, &OptionsWindow::aboutToShow, this, &OptionsWindow::checkDeprecated);
    connect(this, &OptionsWindow::aboutToDelete, this, &OptionsWindow::cancelChanges);
}

void OptionsWindow::selectTab(int index)
{
    m_mainPage->selectTab(index);
}

void OptionsWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setOptWindowGeometry(stateWatcher()->geometry());
    iniUser.setOptWindowMaximized(stateWatcher()->maximized());

    emit ctrl()->afterSaveWindowState(iniUser);

    confManager()->saveIniUser();
}

void OptionsWindow::restoreWindowState()
{
    auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(
            this, QSize(1024, 768), iniUser.optWindowGeometry(), iniUser.optWindowMaximized());

    emit ctrl()->afterRestoreWindowState(iniUser);
}

void OptionsWindow::cancelChanges()
{
    if (ctrl()->anyEdited()) {
        ctrl()->resetEdited();
    }
}

void OptionsWindow::setupController()
{
    connect(ctrl(), &OptionsController::editedChanged, this, &QWidget::setWindowModified);
    connect(ctrl(), &OptionsController::retranslateUi, this, &OptionsWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void OptionsWindow::keyPressEvent(QKeyEvent *event)
{
    WidgetWindow::keyPressEvent(event);

    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_S: // Ctrl+S
        if (event->modifiers() == Qt::ControlModifier) {
            ctrl()->applyChanges();
        }
        break;
    }
}

void OptionsWindow::retranslateUi()
{
    this->unsetLocale();

    this->setWindowTitle(tr("Options") + "[*]");
}

void OptionsWindow::setupUi()
{
    auto layout = ControlUtil::createVLayout();

    m_mainPage = new OptMainPage(ctrl());
    layout->addWidget(m_mainPage);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(800, 500);
}

void OptionsWindow::checkDeprecated()
{
    checkDeprecatedAppGroups();
}

void OptionsWindow::checkDeprecatedAppGroups()
{
    if (!conf()->checkDeprecatedAppGroups()) {
        windowManager()->showConfirmBox(
                [&] { OsUtil::openUrlOrFolder("https://github.com/tnodir/fort/discussions/210"); },
                tr("Please move Texts of Allow/Block fields from App Groups to Wildcard Programs!!!"
                   "\n\n(They are read-only now and will be removed in v4.)"
                   "\n\nDo you want to open a discussion thread in browser?"),
                {}, this);
    }
}
