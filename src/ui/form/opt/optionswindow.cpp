#include "optionswindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/osutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "optionscontroller.h"
#include "pages/optmainpage.h"

OptionsWindow::OptionsWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new OptionsController(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser(), IniUser::optWindowGroup());

    connect(this, &OptionsWindow::aboutToShow, this, &OptionsWindow::checkDeprecated);
}

ConfManager *OptionsWindow::confManager() const
{
    return ctrl()->confManager();
}

IniUser *OptionsWindow::iniUser() const
{
    return ctrl()->iniUser();
}

void OptionsWindow::selectTab(int index)
{
    m_mainPage->selectTab(index);
}

void OptionsWindow::cancelChanges()
{
    if (ctrl()->confToEdit() && ctrl()->anyEdited()) {
        ctrl()->resetEdited();
    }
}

void OptionsWindow::saveWindowState(bool /*wasVisible*/)
{
    iniUser()->setOptWindowGeometry(stateWatcher()->geometry());
    iniUser()->setOptWindowMaximized(stateWatcher()->maximized());

    emit ctrl()->afterSaveWindowState(iniUser());

    confManager()->saveIniUser();
}

void OptionsWindow::restoreWindowState()
{
    stateWatcher()->restore(this, QSize(1024, 768), iniUser()->optWindowGeometry(),
            iniUser()->optWindowMaximized());

    emit ctrl()->afterRestoreWindowState(iniUser());
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

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/cog.png"));

    // Size
    this->setMinimumSize(800, 500);
}

void OptionsWindow::checkDeprecated()
{
    checkDeprecatedAppGroups();
    checkDeprecatedAddressGroups();
}

void OptionsWindow::checkDeprecatedAppGroups()
{
    if (!ctrl()->conf()->checkDeprecatedAppGroups()) {
        ctrl()->windowManager()->showConfirmBox(
                [&] { OsUtil::openUrlOrFolder("https://github.com/tnodir/fort/discussions/210"); },
                tr("Please move Texts of Allow/Block fields from App Groups to Wildcard Programs!!!"
                   "\n\n(They are read-only now and will be removed in v4.)"
                   "\n\nDo you want to open a discussion thread in browser?"),
                {}, this);
    }
}

void OptionsWindow::checkDeprecatedAddressGroups()
{
    if (!ctrl()->conf()->checkDeprecatedAddressGroups()) {
        ctrl()->windowManager()->showConfirmBox(
                [&] { OsUtil::openUrlOrFolder("https://github.com/tnodir/fort/discussions/347"); },
                tr("Please move settings of 'IP Addresses' fields to Global Rules!!!"
                   "\n\n(They are read-only now and will be removed in v4.)"
                   "\n\nDo you want to open a discussion thread in browser?"),
                {}, this);
    }
}
