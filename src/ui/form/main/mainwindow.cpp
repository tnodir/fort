#include "mainwindow.h"

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <fortglobal.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>

#include "paneldock.h"

using namespace Fort;

MainWindow::MainWindow(QWidget *parent) : FormWindow(parent)
{
    setupUi();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &MainWindow::retranslateUi);
    retranslateUi();

    setupFormWindow(iniUser());
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
    saveIniWindowGeometry();

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

void MainWindow::retranslateUi()
{
    this->unsetLocale();

    m_labelHint->setText(tr("Open a panel from the tool bar's menus.\n"
                            "Or move a window here by its menu's \"Dock to Main Window\" item."));

    this->setWindowTitle(QApplication::applicationDisplayName());
}

void MainWindow::setupUi()
{
    setupDockHost();

    auto layout = ControlUtil::createVLayout(/*margin=*/0);
    layout->addWidget(m_dockHost);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 400);
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
