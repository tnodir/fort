#include "aboutpage.h"

#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
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
    m_btCheckUpdate->setText(tr("Check Update"));

    retranslateNewVersionBox();
}

void AboutPage::retranslateNewVersionBox()
{
    m_gbNewVersion->setTitle(m_isNewVersion ? tr("New Version") : tr("No Update"));
}

void AboutPage::setupUi()
{
    auto layout = new QVBoxLayout();

    // New Version Group Box
    setupNewVersionBox();
    setupNewVersionUpdate();
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

    // Buttons
    auto buttonsLayout = setupButtonsLayout();

    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    layout->addWidget(m_labelArea);
    layout->addLayout(buttonsLayout);

    m_gbNewVersion = new QGroupBox();
    m_gbNewVersion->setMinimumWidth(380);
    m_gbNewVersion->setLayout(layout);
}

QLayout *AboutPage::setupButtonsLayout()
{
    // Download
    m_btDownload = ControlUtil::createFlatToolButton(":/icons/download.png");

    connect(m_btDownload, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);

    // Check Update
    m_btCheckUpdate = ControlUtil::createFlatToolButton(
            ":/icons/play.png", [&] { taskManager()->runTask(TaskInfo::UpdateChecker); });

    auto layout = new QHBoxLayout();
    layout->setSpacing(10);

    layout->addStretch();
    layout->addWidget(m_btDownload);
    layout->addWidget(m_btCheckUpdate);
    layout->addStretch();

    return layout;
}

void AboutPage::setupNewVersionUpdate()
{
    const auto refreshNewVersion = [&] {
        auto updateChecker = taskManager()->taskInfoUpdateChecker();
        m_isNewVersion = updateChecker->isNewVersion();

        m_labelArea->setVisible(m_isNewVersion);
        m_labelRelease->setText(updateChecker->releaseText());

        m_btDownload->setVisible(m_isNewVersion);
        m_btDownload->setWindowFilePath(updateChecker->downloadUrl());
        m_btDownload->setToolTip(updateChecker->downloadUrl());

        m_btCheckUpdate->setToolTip(DateUtil::localeDateTime(updateChecker->lastSuccess()));

        retranslateNewVersionBox();
    };

    refreshNewVersion();

    connect(taskManager(), &TaskManager::appVersionUpdated, this, refreshNewVersion);
}
