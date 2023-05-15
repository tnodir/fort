#include "aboutpage.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <task/taskinfoupdatechecker.h>
#include <task/taskmanager.h>
#include <util/iconcache.h>

AboutPage::AboutPage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void AboutPage::onRetranslateUi()
{
    m_gbNewVersion->setTitle(tr("New Version"));
    m_btNewVersion->setText(tr("Download"));
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
    auto colLayout = new QVBoxLayout();
    colLayout->setSpacing(10);

    // Label
    m_labelNewVersion = ControlUtil::createLabel();
    m_labelNewVersion->setTextFormat(Qt::MarkdownText);
    m_labelNewVersion->setWordWrap(true);
    m_labelNewVersion->setOpenExternalLinks(true);
    colLayout->addWidget(m_labelNewVersion);

    // Button
    m_btNewVersion = ControlUtil::createLinkButton(":/icons/download_for_windows.png");

    connect(m_btNewVersion, &QAbstractButton::clicked, ctrl(), &BaseController::onLinkClicked);

    colLayout->addWidget(m_btNewVersion, 0, Qt::AlignHCenter);

    m_gbNewVersion = new QGroupBox();
    m_gbNewVersion->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_gbNewVersion->setMinimumWidth(380);
    m_gbNewVersion->setLayout(colLayout);
}

void AboutPage::setupNewVersionUpdate()
{
    const auto refreshNewVersion = [&] {
        auto updateChecker = taskManager()->taskInfoUpdateChecker();
        m_gbNewVersion->setVisible(updateChecker->isNewVersion());
        m_labelNewVersion->setText(updateChecker->releaseText());
        m_btNewVersion->setWindowFilePath(updateChecker->downloadUrl());
        m_btNewVersion->setToolTip(updateChecker->downloadUrl());
    };

    refreshNewVersion();

    connect(taskManager(), &TaskManager::appVersionUpdated, this, refreshNewVersion);
}
