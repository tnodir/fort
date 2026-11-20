#include "mainwindow.h"

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/logowidget.h>
#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>

#include "mainmenubar.h"
#include "maintoolbar.h"
#include "paneldock.h"

using namespace Fort;

namespace {

inline constexpr int DOCKS_STATE_VERSION = 1;

struct DockWindow
{
    WindowCode code;
    QString (*iniGroup)();
};

// The saved docks are found by their windows' ini groups
inline constexpr DockWindow dockWindows[] = {
    { WindowPrograms, &IniUser::progWindowGroup },
    { WindowRules, &IniUser::ruleWindowGroup },
    { WindowZones, &IniUser::zoneWindowGroup },
    { WindowGroups, &IniUser::groupWindowGroup },
    { WindowSpeedLimits, &IniUser::speedLimitWindowGroup },
    { WindowTimePeriods, &IniUser::timePeriodWindowGroup },
    { WindowServices, &IniUser::serviceWindowGroup },
    { WindowConnections, &IniUser::connWindowGroup },
    { WindowTraffic, &IniUser::statWindowGroup },
};

constexpr quint32 dockWindowsMask()
{
    quint32 mask = 0;
    for (const auto &dockWindow : dockWindows) {
        mask |= dockWindow.code;
    }
    return mask;
}

static_assert(dockWindowsMask() == WindowDockable, "dockWindows[] must list the WindowDockable");

WindowCode dockWindowCode(const QString &iniGroup)
{
    for (const auto &dockWindow : dockWindows) {
        if (dockWindow.iniGroup() == iniGroup) {
            return dockWindow.code;
        }
    }

    return WindowNone;
}

}

MainWindow::MainWindow(QWidget *parent) : FormWindow(parent)
{
    setupUi();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &MainWindow::retranslateUi);
    retranslateUi();

    setupFormWindow(iniUser());

    // After the window is shown: the docked windows are shown in it
    QMetaObject::invokeMethod(this, &MainWindow::restoreDocks, Qt::QueuedConnection);
}

QString MainWindow::helpSection() const
{
    // The docked window's, which has the focus
    const auto dock = currentDock();

    return dock ? dock->formWindow()->helpSection() : "main-window";
}

QString MainWindow::iniGroup() const
{
    return IniUser::mainWindowGroup();
}

void MainWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    saveIniWindowGeometry();

    iniUser.setMainWindowDocks(dockGroups());
    iniUser.setMainWindowDocksState(m_dockHost->saveState(DOCKS_STATE_VERSION));

    confManager()->saveIniUser();
}

void MainWindow::restoreWindowState()
{
    restoreIniWindowGeometry(QSize(1024, 768));
}

void MainWindow::onWindowClosed()
{
    windowManager()->closeDockedWindows();
}

bool MainWindow::checkAboutToClose()
{
    if (!iniUser().mainWindowAutoShowWindow())
        return true;

    auto windowManager = Fort::windowManager();
    auto trayIcon = windowManager->trayIcon();

    if (trayIcon->isVisible())
        return true;

    if (windowManager->isAppQuitting())
        return true;

    emit windowManager->requestQuit();

    return false;
}

QList<PanelDock *> MainWindow::docks() const
{
    QList<PanelDock *> list;

    // The removed docks are deleted later
    const auto docks = m_dockHost->findChildren<PanelDock *>(QString(), Qt::FindDirectChildrenOnly);
    for (PanelDock *dock : docks) {
        if (dock->formWindow()) {
            list.append(dock);
        }
    }

    return list;
}

PanelDock *MainWindow::currentDock() const
{
    const auto w = focusWidget();
    if (!w)
        return nullptr;

    const auto docks = this->docks();
    for (PanelDock *dock : docks) {
        if (dock->isAncestorOf(w))
            return dock;
    }

    return nullptr;
}

PanelDock *MainWindow::addDock(FormWindow *w)
{
    // As a tab of the current dock or of the last one
    const auto docks = this->docks();
    auto tabDock = currentDock();
    if (!tabDock && !docks.isEmpty()) {
        tabDock = docks.last();
    }

    auto dock = new PanelDock(w, m_dockHost);
    m_dockHost->addDockWidget(Qt::LeftDockWidgetArea, dock);

    if (tabDock) {
        m_dockHost->tabifyDockWidget(tabDock, dock);
    }

    updateHint();

    return dock;
}

void MainWindow::removeDock(PanelDock *dock)
{
    m_dockHost->removeDockWidget(dock);

    dock->takeFormWindow();
    dock->deleteLater();

    updateHint();
}

void MainWindow::resetDocksLayout()
{
    const auto docks = this->docks();
    if (docks.isEmpty())
        return;

    // The current tab stays current
    const auto shownDock = currentDock();
    const auto firstDock = docks.first();

    for (PanelDock *dock : docks) {
        m_dockHost->removeDockWidget(dock);
    }

    for (PanelDock *dock : docks) {
        m_dockHost->addDockWidget(Qt::LeftDockWidgetArea, dock);
        dock->show();

        if (dock != firstDock) {
            m_dockHost->tabifyDockWidget(firstDock, dock);
        }
    }

    if (shownDock) {
        shownDock->raise();
    }
}

QStringList MainWindow::dockGroups() const
{
    QStringList list;

    const auto docks = this->docks();
    for (PanelDock *dock : docks) {
        list.append(dock->objectName());
    }

    return list;
}

void MainWindow::restoreDocks()
{
    if (!isVisible())
        return;

    const auto &iniUser = Fort::iniUser();

    // The window shown on request stays current
    const auto shownDocks = docks();

    const QStringList groups = iniUser.mainWindowDocks();
    for (const QString &group : groups) {
        const WindowCode code = dockWindowCode(group);
        if (code != WindowNone) {
            windowManager()->showWindowByCode(code, /*activate=*/false);
        }
    }

    m_dockHost->restoreState(iniUser.mainWindowDocksState(), DOCKS_STATE_VERSION);

    for (PanelDock *dock : shownDocks) {
        dock->raise();
    }
}

void MainWindow::retranslateUi()
{
    this->unsetLocale();

    m_labelHint->setText(tr("Open a panel from the tool bar's menus.\n"
                            "Or move a window here by its menu's \"Dock to My Fort\" item."));

    this->setWindowTitle(QApplication::applicationDisplayName());
}

void MainWindow::setupUi()
{
    setupDockHost();

    auto layout = ControlUtil::createVLayout(/*margin=*/0);
    layout->setMenuBar(setupHeader());
    layout->addWidget(m_dockHost);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
}

QWidget *MainWindow::setupHeader()
{
    auto logo = new LogoWidget(/*compact=*/true);
    logo->setContentsMargins(0, 0, 6, 0);

    // The logo beside the menu bar and the tool bar
    auto layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->setColumnStretch(0, 1);

    auto toolBar = new MainToolBar();

    layout->addWidget(new MainMenuBar(this), 0, 0);
    layout->addWidget(toolBar, 1, 0);
    layout->addWidget(logo, 0, 1, 2, 1);

    return ControlUtil::wrapToWidget(layout);
}

void MainWindow::setupDockHost()
{
    m_labelHint = ControlUtil::createLabel();
    m_labelHint->setAlignment(Qt::AlignCenter);
    m_labelHint->setForegroundRole(QPalette::PlaceholderText);
    m_labelHint->setWordWrap(true);

    // QMainWindow's constructor adds Qt::Window to the flags
    m_dockHost = new QMainWindow(this);
    m_dockHost->setWindowFlags(Qt::Widget);
    m_dockHost->setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks
            | QMainWindow::AllowTabbedDocks);
    m_dockHost->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    m_dockHost->setCentralWidget(m_labelHint);
}

void MainWindow::updateHint()
{
    m_labelHint->setVisible(docks().isEmpty());
}
