#include "aboutpage.h"

#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <fortsettings.h>
#include <manager/autoupdatemanager.h>
#include <task/taskinfoupdatechecker.h>
#include <task/taskmanager.h>
#include <util/dateutil.h>
#include <util/iconcache.h>

AboutPage::AboutPage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void AboutPage::onRetranslateUi()
{
    m_btDownload->setText(tr("Download"));
    m_btInstall->setText(tr("Install"));
    m_btCheckUpdate->setText(tr("Check for update"));

    retranslateNewVersionBox();
}

void AboutPage::retranslateNewVersionBox()
{
    const auto title = m_isNewVersion ? tr("New Version") : tr("No Update");
    const auto lastTime = DateUtil::localeDateTime(m_lastCheckTime);

    m_gbNewVersion->setTitle(QString("%1 — %2").arg(lastTime, title));
}

void AboutPage::setupUi()
{
    // New Version Group Box
    setupNewVersionBox();

    setupNewVersionUpdate();
    setupAutoUpdate();

    auto layout = new QVBoxLayout();
    layout->addWidget(m_gbNewVersion, 0, Qt::AlignHCenter);
    layout->addStretch();

    this->setLayout(layout);
}

void AboutPage::setupNewVersionBox()
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

QLayout *AboutPage::setupButtonsLayout()
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

void AboutPage::setupNewVersionUpdate()
{
    const auto refreshNewVersion = [&] {
        auto taskInfo = taskManager()->taskInfoUpdateChecker();

        m_isNewVersion = taskInfo->isNewVersion();
        m_lastCheckTime = taskInfo->lastSuccess();

        m_labelArea->setVisible(m_isNewVersion);
        m_labelRelease->setText(taskInfo->releaseText());

        m_progressBar->setRange(0, taskInfo->downloadSize());
        m_btDownload->setVisible(m_isNewVersion && !m_btInstall->isVisible());

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

void AboutPage::setupAutoUpdate()
{
    const auto refreshAutoUpdate = [&] {
        auto manager = autoUpdateManager();

        const bool isNewVersion = manager->isNewVersion();
        const bool isDownloaded = manager->isDownloaded();
        const bool isDownloading = manager->isDownloading();
        const bool isDownloadActive = (isDownloading || isDownloaded);

        if (!isDownloading || isDownloaded) {
            m_progressBar->setValue(
                    isDownloaded ? m_progressBar->maximum() : m_progressBar->minimum());
        }
        m_progressBar->setVisible(isDownloadActive);

        m_btDownload->setVisible(isNewVersion && !isDownloadActive);
        m_btInstall->setVisible(isNewVersion && isDownloaded);

        m_btInstall->setToolTip(isDownloaded ? manager->installerPath() : QString());
    };

    refreshAutoUpdate();

    connect(autoUpdateManager(), &AutoUpdateManager::flagsChanged, this, refreshAutoUpdate);

    connect(autoUpdateManager(), &AutoUpdateManager::bytesReceivedChanged, m_progressBar,
            &QProgressBar::setValue);
}
