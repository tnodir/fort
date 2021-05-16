#include "optionswindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include "../../conf/confmanager.h"
#include "../../conf/firewallconf.h"
#include "../../user/iniuser.h"
#include "../../util/guiutil.h"
#include "../../util/window/widgetwindowstatewatcher.h"
#include "optionscontroller.h"
#include "pages/mainpage.h"

OptionsWindow::OptionsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new OptionsController(fortManager, this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

ConfManager *OptionsWindow::confManager() const
{
    return ctrl()->confManager();
}

FirewallConf *OptionsWindow::conf() const
{
    return confManager()->conf();
}

IniOptions *OptionsWindow::ini() const
{
    return &conf()->ini();
}

IniUser *OptionsWindow::iniUser() const
{
    return ctrl()->iniUser();
}

void OptionsWindow::cancelChanges()
{
    if (ctrl()->conf() && ctrl()->conf()->anyEdited()) {
        emit ctrl()->cancelChanges(ini());
    }
}

void OptionsWindow::saveWindowState()
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

    m_mainPage = new MainPage(ctrl());
    layout->addWidget(m_mainPage);

    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/sheild-96.png", ":/icons/cog.png"));

    // Size
    this->setMinimumSize(800, 500);
}
