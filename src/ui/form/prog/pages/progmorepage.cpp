#include "progmorepage.h"

#include <QCheckBox>

#include <form/controls/controlutil.h>
#include <form/prog/programeditcontroller.h>

ProgMorePage::ProgMorePage(ProgramEditController *ctrl, QWidget *parent) :
    ProgBasePage(ctrl, parent)
{
    setupUi();
}

void ProgMorePage::onPageInitialize(const App &app)
{
    m_cbKillChild->setChecked(app.killChild);
    m_cbParked->setChecked(app.parked);
    m_cbLogAllowedConn->setChecked(app.logAllowedConn);
    m_cbLogBlockedConn->setChecked(app.logBlockedConn);
}

void ProgMorePage::onRetranslateUi()
{
    m_cbKillChild->setText(tr("Kill child processes"));

    m_cbParked->setText(tr("Parked"));
    m_cbParked->setToolTip(tr("Don't purge as obsolete"));

    m_cbLogAllowedConn->setText(tr("Collect allowed connections"));
    m_cbLogBlockedConn->setText(tr("Collect blocked connections"));
}

void ProgMorePage::setupUi()
{
    setupOptions();
    setupLogConn();

    // Main Layout
    auto layout = new QVBoxLayout();
    layout->addWidget(m_cbKillChild);
    layout->addWidget(m_cbParked);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_cbLogAllowedConn);
    layout->addWidget(m_cbLogBlockedConn);
    layout->addStretch();

    this->setLayout(layout);
}

void ProgMorePage::setupOptions()
{
    // Kill Child
    m_cbKillChild = ControlUtil::createCheckBox(":/icons/scull.png");

    connect(m_cbKillChild, &QCheckBox::clicked, ctrl(),
            &ProgramEditController::warnDangerousOption);

    // Parked
    m_cbParked = ControlUtil::createCheckBox(":/icons/parking.png");
}

void ProgMorePage::setupLogConn()
{
    // Log Allowed Connections
    m_cbLogAllowedConn = new QCheckBox();

    // Log Blocked Connections
    m_cbLogBlockedConn = new QCheckBox();
}

void ProgMorePage::fillApp(App &app) const
{
    app.killChild = m_cbKillChild->isChecked();
    app.parked = m_cbParked->isChecked();
    app.logAllowedConn = m_cbLogAllowedConn->isChecked();
    app.logBlockedConn = m_cbLogBlockedConn->isChecked();
}
