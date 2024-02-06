#include "optionswindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/osutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "optionscontroller.h"
#include "pages/optmainpage.h"

OptionsWindow::OptionsWindow(QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new OptionsController(this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();

    connect(this, &OptionsWindow::aboutToShow, this, &OptionsWindow::checkDeprecatedAppGroups);
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
    iniUser()->setOptWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setOptWindowMaximized(m_stateWatcher->maximized());

    emit ctrl()->afterSaveWindowState(iniUser());

    confManager()->saveIniUser();
}

void OptionsWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(1024, 768), iniUser()->optWindowGeometry(),
            iniUser()->optWindowMaximized());

    emit ctrl()->afterRestoreWindowState(iniUser());
}

void OptionsWindow::setupController()
{
    ctrl()->initialize();

    connect(ctrl(), &OptionsController::editedChanged, this, &QWidget::setWindowModified);
    connect(ctrl(), &OptionsController::retranslateUi, this, &OptionsWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void OptionsWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
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
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

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
