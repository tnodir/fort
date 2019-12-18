#include "optionswindow.h"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QPushButton>
#include <QStackedLayout>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "../../fortsettings.h"
#include "../../task/taskmanager.h"
#include "../../task/taskinfoupdatechecker.h"

namespace {

bool openUrlExternally(const QUrl &url)
{
    return QDesktopServices::openUrl(url);
}

}

OptionsWindow::OptionsWindow(FortManager *fortManager,
                             QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(fortManager)
{
    setupUi();
    retranslateUi();

    ctrl()->initialize();
}

void OptionsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(2, 2, 2, 2);
    //layout->setMargin(0);

    // Main Tab Bar
    m_tabBar = new QTabBar();
    m_tabBar->addTab(QIcon(":/images/cog.png"), QString());
    m_tabBar->addTab(QIcon(":/images/link.png"), QString());
    m_tabBar->addTab(QIcon(":/images/application_double.png"), QString());
    m_tabBar->addTab(QIcon(":/images/application.png"), QString());
    m_tabBar->addTab(QIcon(":/images/chart_line.png"), QString());
    m_tabBar->addTab(QIcon(":/images/clock.png"), QString());
    layout->addWidget(m_tabBar);

    m_stackLayout = new QStackedLayout();
    layout->addLayout(m_stackLayout);

    // Dialog butons
    auto buttonsLayout = setupDialogButtons();
    layout->addLayout(buttonsLayout);

    this->setLayout(layout);
}

QLayout *OptionsWindow::setupDialogButtons()
{
    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(2, 2, 2, 2);

    m_logsButton = createLinkButton(":/images/folder_error.png", fortSettings()->logsPath());
    m_profileButton = createLinkButton(":/images/folder_user.png", fortSettings()->profilePath());
    m_statButton = createLinkButton(":/images/folder_database.png", fortSettings()->statPath());
    m_releasesButton = createLinkButton(":/images/server_go.png", fortSettings()->appUpdatesUrl());
    m_newVersionButton = createLinkButton(":/images/server_compressed.png");

    connect(m_logsButton, &QAbstractButton::clicked, this, &OptionsWindow::onLinkClicked);
    connect(m_profileButton, &QAbstractButton::clicked, this, &OptionsWindow::onLinkClicked);
    connect(m_statButton, &QAbstractButton::clicked, this, &OptionsWindow::onLinkClicked);
    connect(m_releasesButton, &QAbstractButton::clicked, this, &OptionsWindow::onLinkClicked);
    connect(m_newVersionButton, &QAbstractButton::clicked, this, &OptionsWindow::onLinkClicked);

    setupNewVersionButton();

    buttonsLayout->addWidget(m_logsButton);
    buttonsLayout->addWidget(m_profileButton);
    buttonsLayout->addWidget(m_statButton);
    buttonsLayout->addWidget(m_releasesButton);
    buttonsLayout->addWidget(m_newVersionButton);

    buttonsLayout->addStretch(1);

    m_okButton = new QPushButton(QIcon(":/images/tick.png"), QString());
    m_applyButton = new QPushButton(QIcon(":/images/accept.png"), QString());
    m_cancelButton = new QPushButton(QIcon(":/images/cancel.png"), QString());

    connect(m_okButton, &QAbstractButton::clicked, ctrl(), &OptionsController::saveChanges);
    connect(m_applyButton, &QAbstractButton::clicked, ctrl(), &OptionsController::applyChanges);
    connect(m_cancelButton, &QAbstractButton::clicked, ctrl(), &OptionsController::closeWindow);

    buttonsLayout->addWidget(m_okButton);
    buttonsLayout->addWidget(m_applyButton);
    buttonsLayout->addWidget(m_cancelButton);

    return buttonsLayout;
}

void OptionsWindow::setupNewVersionButton()
{
    auto updateChecker = taskManager()->taskInfoUpdateChecker();

    const auto refreshNewVersionButton = [&] {
        m_newVersionButton->setVisible(!updateChecker->version().isEmpty());
        m_newVersionButton->setWindowFilePath(updateChecker->downloadUrl());
        m_newVersionButton->setToolTip(updateChecker->releaseText());
    };

    refreshNewVersionButton();

    connect(updateChecker, &TaskInfoUpdateChecker::versionChanged, this, refreshNewVersionButton);
}

void OptionsWindow::retranslateUi()
{
    m_tabBar->setTabText(0, tr("Options"));
    m_tabBar->setTabText(1, tr("IPv4 Addresses"));
    m_tabBar->setTabText(2, tr("Program Groups"));
    m_tabBar->setTabText(3, tr("Programs"));
    m_tabBar->setTabText(4, tr("Statistics"));
    m_tabBar->setTabText(5, tr("Schedule"));

    m_logsButton->setText(tr("Logs"));
    m_profileButton->setText(tr("Profile"));
    m_statButton->setText(tr("Statistics"));
    m_releasesButton->setText(tr("Releases"));
    m_newVersionButton->setText(tr("New Version!"));

    m_okButton->setText(tr("OK"));
    m_applyButton->setText(tr("Apply"));
    m_cancelButton->setText(tr("Cancel"));
}

void OptionsWindow::closeEvent(QCloseEvent *event)
{
    if (isVisible()) {
        event->ignore();
        ctrl()->closeWindow();
    }
}

void OptionsWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape
            && event->modifiers() == Qt::NoModifier) {
        ctrl()->closeWindow();
    }
}

FortSettings *OptionsWindow::fortSettings()
{
    return ctrl()->fortSettings();
}

TaskManager *OptionsWindow::taskManager()
{
    return ctrl()->taskManager();
}

void OptionsWindow::onLinkClicked()
{
    auto button = qobject_cast<QAbstractButton *>(sender());
    if (button) {
        openUrlExternally(QUrl::fromLocalFile(button->windowFilePath()));
    }
}

QAbstractButton *OptionsWindow::createLinkButton(const QString &iconPath,
                                                 const QString &linkPath,
                                                 const QString &toolTip)
{
    auto button = new QPushButton(QIcon(iconPath), QString());
    button->setWindowFilePath(linkPath);
    button->setToolTip(!toolTip.isEmpty() ? toolTip : linkPath);
    return button;
}
