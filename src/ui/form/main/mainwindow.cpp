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
    return "main-window";
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
