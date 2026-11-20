#include "windowmenubutton.h"

#include <QMenu>
#include <QStyleOptionToolButton>

#include <fortglobal.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>
#include <util/osutil.h>

#include "applinks.h"
#include "controlutil.h"
#include "formwindow.h"

using namespace Fort;

WindowMenuButton::WindowMenuButton(QWidget *parent) : ToolButton(parent)
{
    setupUi();
}

void WindowMenuButton::setDocked(bool v)
{
    if (m_docked == v)
        return;

    m_docked = v;

    retranslateUi();
}

void WindowMenuButton::initStyleOption(QStyleOptionToolButton *option) const
{
    ToolButton::initStyleOption(option);

    // The icon is the menu's arrow
    option->features &= ~QStyleOptionToolButton::HasMenu;
}

void WindowMenuButton::setupUi()
{
    setIcon(IconCache::icon(":/icons/bullet_arrow_down.png"));

    ControlUtil::setToolButtonFlat(this);

    setupMenu();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &WindowMenuButton::retranslateUi);
    retranslateUi();
}

void WindowMenuButton::setupMenu()
{
    auto menu = ControlUtil::createMenu(this);

    m_actDock = menu->addAction(QString());

    menu->addSeparator();

    m_actHelp = menu->addAction(IconCache::icon(AppLinks::iconPath(AppLinks::LinkHelp)), QString());

    m_actClose = menu->addAction(ControlUtil::clearIcon(this), QString());

    connect(m_actDock, &QAction::triggered, this, &WindowMenuButton::onDockTriggered);
    connect(m_actHelp, &QAction::triggered, this, &WindowMenuButton::onHelpTriggered);
    connect(m_actClose, &QAction::triggered, this, &WindowMenuButton::onCloseTriggered);

    setPopupMode(QToolButton::InstantPopup);
    setMenu(menu);
}

void WindowMenuButton::retranslateUi()
{
    setToolTip(tr("Window Menu"));

    m_actDock->setText(m_docked ? tr("Undock from Main Window") : tr("Dock to Main Window"));
    m_actHelp->setText(tr("Help"));
    m_actClose->setText(tr("Close"));
}

void WindowMenuButton::onDockTriggered()
{
    auto windowManager = Fort::windowManager();

    const WindowCode code = m_formWindow->windowCode();

    if (m_docked) {
        windowManager->undockWindowByCode(code);
    } else {
        windowManager->dockWindowByCode(code);
    }
}

void WindowMenuButton::onHelpTriggered()
{
    OsUtil::openUserGuide(m_formWindow->helpSection());
}

void WindowMenuButton::onCloseTriggered()
{
    windowManager()->closeWindowByCode(m_formWindow->windowCode());
}
