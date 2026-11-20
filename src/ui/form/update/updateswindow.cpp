#include "updateswindow.h"

#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <manager/autoupdatemanager.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <task/taskinfoupdatechecker.h>
#include <task/taskmanager.h>
#include <user/iniuser.h>
#include <util/dateutil.h>
#include <util/iconcache.h>

using namespace Fort;

UpdatesWindow::UpdatesWindow(QWidget *parent) : FormWindow(parent)
{
    setupUi();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &UpdatesWindow::retranslateUi);
    retranslateUi();

    setupFormWindow(iniUser());
}

QString UpdatesWindow::iniGroup() const
{
    return IniUser::updatesWindowGroup();
}

void UpdatesWindow::saveWindowState(bool /*wasVisible*/)
{
    saveIniWindowGeometry();

    confManager()->saveIniUser();
}

void UpdatesWindow::restoreWindowState()
{
    restoreIniWindowGeometry(QSize(500, 400));
}

void UpdatesWindow::retranslateUi()
{
    this->unsetLocale();

    m_btDownload->setText(tr("Download"));
    m_btInstall->setText(tr("Install"));
    m_btCheckUpdate->setText(tr("Check for Updates"));

    retranslateNewVersionBox();

    this->setWindowTitle(tr("Updates"));
}

void UpdatesWindow::retranslateNewVersionBox()
{
    const auto title = m_isNewVersion
            ? tr("New Version")
            : (m_keepCurrentVersion ? tr("Current Version") : tr("No Update"));
    const auto lastTime = DateUtil::localeDateTime(m_lastCheckTime);

    m_gbNewVersion->setTitle(QString("%1 — %2").arg(lastTime, title));
}

void UpdatesWindow::setupUi()
{
    // New Version Group Box
    setupNewVersionBox();

    setupNewVersionUpdate();
    setupAutoUpdate();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_gbNewVersion);
    layout->addStretch();

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(400, 200);
}

void UpdatesWindow::setupNewVersionBox()
{
    // Label
    m_labelRelease = ControlUtil::createLabel();
    m_labelRelease->setTextFormat(Qt::MarkdownText);
    m_labelRelease->setWordWrap(true);
    m_labelRelease->setOpenExternalLinks(true);

    m_labelArea = ControlUtil::wrapToScrollArea(m_labelRelease);
    m_labelArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    // Progress Bar
    m_progressBar = new QProgressBar();

    // Buttons
    auto buttonsLayout = setupButtonsLayout();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    layout->addWidget(m_labelArea);
    layout->addWidget(m_progressBar);
    layout->addLayout(buttonsLayout);

    m_gbNewVersion = new QGroupBox();
    m_gbNewVersion->setMinimumWidth(380);
    m_gbNewVersion->setLayout(layout);
}

QLayout *UpdatesWindow::setupButtonsLayout()
{
    // Download
    m_btDownload = ControlUtil::createFlatToolButton(
            ":/icons/download.png", [&] { autoUpdateManager()->startDownload(); });

    // Install
    m_btInstall = ControlUtil::createFlatToolButton(
            ":/icons/tick.png", [&] { autoUpdateManager()->runInstaller(); });

    m_btInstall->setVisible(false);

    // Check Update
    m_btCheckUpdate = ControlUtil::createFlatToolButton(
            ":/icons/play.png", [&] { taskManager()->runTask(TaskInfo::UpdateChecker); });

    auto layout = ControlUtil::createHLayoutByWidgets({ /*stretch*/ nullptr, m_btDownload,
            m_btInstall, m_btCheckUpdate, /*stretch*/ nullptr });
    layout->setSpacing(6);

    return layout;
}

void UpdatesWindow::setupNewVersionUpdate()
{
    const auto refreshNewVersion = [&] {
        auto taskInfo = taskManager()->taskInfoUpdateChecker();

        m_keepCurrentVersion = ini().updateKeepCurrentVersion();
        m_isNewVersion = taskInfo->isNewVersion();
        m_lastCheckTime = taskInfo->lastSuccess();

        const bool hasUpdate = m_keepCurrentVersion || m_isNewVersion;

        m_labelArea->setVisible(hasUpdate);
        m_labelRelease->setText(taskInfo->releaseText());

        m_progressBar->setRange(0, taskInfo->downloadSize());
        m_btDownload->setVisible(hasUpdate && !m_btInstall->isVisible());

        retranslateNewVersionBox();
    };

    refreshNewVersion();

    connect(taskManager(), &TaskManager::appVersionUpdated, this, refreshNewVersion);

    connect(taskManager(), &TaskManager::taskFinished, this, [=, this](qint8 taskType) {
        if (taskType == TaskInfo::UpdateChecker) {
            refreshNewVersion();
        }
    });
}

void UpdatesWindow::setupAutoUpdate()
{
    refreshAutoUpdate();

    connect(autoUpdateManager(), &AutoUpdateManager::keepCurrentVersionChanged, this,
            &UpdatesWindow::refreshAutoUpdate);
    connect(autoUpdateManager(), &AutoUpdateManager::flagsChanged, this,
            &UpdatesWindow::refreshAutoUpdate);

    connect(autoUpdateManager(), &AutoUpdateManager::bytesReceivedChanged, m_progressBar,
            &QProgressBar::setValue);
}

void UpdatesWindow::refreshAutoUpdate()
{
    auto manager = autoUpdateManager();

    const bool hasUpdate = manager->hasUpdate();
    const bool isDownloaded = manager->isDownloaded();
    const bool isDownloading = manager->isDownloading();
    const bool isDownloadActive = (isDownloading || isDownloaded);

    refreshDownloadProgress(isDownloading, isDownloaded);
    m_progressBar->setVisible(hasUpdate && isDownloadActive);

    m_btDownload->setVisible(hasUpdate && !isDownloadActive);
    m_btInstall->setVisible(hasUpdate && isDownloaded);

    m_btInstall->setToolTip(isDownloaded ? manager->installerPath() : QString());

    m_btCheckUpdate->setVisible(!isDownloading);
}

void UpdatesWindow::refreshDownloadProgress(bool isDownloading, bool isDownloaded)
{
    if (isDownloading && !isDownloaded)
        return;

    m_progressBar->setValue(isDownloaded ? m_progressBar->maximum() : m_progressBar->minimum());
}
