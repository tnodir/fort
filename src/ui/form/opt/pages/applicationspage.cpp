#include "applicationspage.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "../../../conf/appgroup.h"
#include "../../../conf/firewallconf.h"
#include "../../../util/net/netutil.h"
#include "../../controls/checkspincombo.h"
#include "../../controls/controlutil.h"
#include "../../controls/tabbar.h"
#include "../optionscontroller.h"

namespace {

const ValuesList speedLimitValues = {
    10, 0, 20, 30, 50, 75, 100, 150, 200, 300, 500, 900,
    1024, qRound(1.5 * 1024), 2 * 1024, 3 * 1024, 5 * 1024, qRound(7.5 * 1024),
    10 * 1024, 15 * 1024, 20 * 1024, 30 * 1024, 50 * 1024
};

}

ApplicationsPage::ApplicationsPage(OptionsController *ctrl,
                                   QWidget *parent) :
    BasePage(ctrl, parent)
{
    setupUi();
}

void ApplicationsPage::onRetranslateUi()
{
    m_editGroupName->setPlaceholderText(tr("Group Name"));
    m_btAddGroup->setText(tr("Add Group"));
    m_btRenameGroup->setText(tr("Rename Group"));

    m_cbBlockAll->setText(tr("Block All"));
    m_cbAllowAll->setText(tr("Allow All"));

    m_btGroupOptions->setText(tr("Options"));
    m_cscLimitIn->checkBox()->setText(tr("Download speed limit, KiB/s:"));
    m_cscLimitOut->checkBox()->setText(tr("Upload speed limit, KiB/s:"));
    retranslateGroupLimits();
    m_cbFragmentPacket->setText(tr("Fragment first TCP packet"));
}

void ApplicationsPage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Tab Bar
    setupTabBar();
    layout->addWidget(m_tabBar);

    // App Group
    auto groupHeader = setupGroupHeader();
    layout->addLayout(groupHeader);

    setupAppGroup();

    layout->addStretch();

    this->setLayout(layout);
}

QLayout *ApplicationsPage::setupHeader()
{
    auto layout = new QHBoxLayout();

    m_editGroupName = new QLineEdit();
    m_editGroupName->setFixedWidth(200);

    setupAddGroup();
    setupRenameGroup();

    setupBlockAllowAll();

    layout->addWidget(m_editGroupName);
    layout->addWidget(m_btAddGroup);
    layout->addWidget(m_btRenameGroup);
    layout->addStretch();
    layout->addWidget(m_cbBlockAll);
    layout->addWidget(m_cbAllowAll);

    return layout;
}

void ApplicationsPage::setupAddGroup()
{
    m_btAddGroup = ControlUtil::createButton(":/images/application_add.png", [&] {
        const auto text = m_editGroupName->text();
        if (text.isEmpty()) {
            m_editGroupName->setFocus();
            return;
        }

        conf()->addAppGroupByName(text);
        resetGroupName();

        const int tabIndex = m_tabBar->addTab(text);
        m_tabBar->setCurrentIndex(tabIndex);

        ctrl()->setConfEdited(true);
    });

    const auto refreshAddGroup = [&] {
        m_btAddGroup->setEnabled(appGroupsCount() < 16);
    };

    refreshAddGroup();

    connect(conf(), &FirewallConf::appGroupsChanged, this, refreshAddGroup);
}

void ApplicationsPage::setupRenameGroup()
{
    m_btRenameGroup = ControlUtil::createButton(":/images/application_edit.png", [&] {
        const auto text = m_editGroupName->text();
        if (text.isEmpty()) {
            m_editGroupName->setFocus();
            return;
        }

        const int tabIndex = m_tabBar->currentIndex();
        m_tabBar->setTabText(tabIndex, text);

        appGroup()->setName(text);
        resetGroupName();

        ctrl()->setConfEdited(true);
    });
}

void ApplicationsPage::setupBlockAllowAll()
{
    m_cbBlockAll = ControlUtil::createCheckBox(conf()->appBlockAll(), [&](bool checked) {
        conf()->setAppBlockAll(checked);
        ctrl()->setConfFlagsEdited(true);
    });
    m_cbAllowAll = ControlUtil::createCheckBox(conf()->appAllowAll(), [&](bool checked) {
        conf()->setAppAllowAll(checked);
        ctrl()->setConfFlagsEdited(true);
    });

    const auto refreshBlockAllowAllEnabled = [&] {
        const bool blockAll = m_cbBlockAll->isChecked();
        const bool allowAll = m_cbAllowAll->isChecked();

        m_cbBlockAll->setEnabled(blockAll || !allowAll);
        m_cbAllowAll->setEnabled(!blockAll || allowAll);
    };

    refreshBlockAllowAllEnabled();

    connect(m_cbBlockAll, &QCheckBox::toggled, this, refreshBlockAllowAllEnabled);
    connect(m_cbAllowAll, &QCheckBox::toggled, this, refreshBlockAllowAllEnabled);
}

void ApplicationsPage::setupTabBar()
{
    m_tabBar = new TabBar();
    m_tabBar->setTabMinimumWidth(120);
    m_tabBar->setShape(QTabBar::TriangularNorth);
    m_tabBar->setExpanding(false);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);

    for (const auto appGroup : conf()->appGroupsList()) {
        addTab(appGroup->name());
    }

    connect(m_tabBar, &QTabBar::tabCloseRequested, [&](int index) {
        conf()->removeAppGroup(index, index);

        if (m_tabBar->count() > 1) {
            m_tabBar->removeTab(index);
        } else {
            // Reset alone tab to default one
            m_tabBar->setTabText(0, appGroup()->name());
        }

        ctrl()->setConfEdited(true);
    });
    connect(m_tabBar, &QTabBar::tabMoved, [&](int from, int to) {
        conf()->moveAppGroup(from, to);
        ctrl()->setConfEdited(true);
    });
}

int ApplicationsPage::addTab(const QString &text)
{
    const int tabIndex = m_tabBar->addTab(text);
    m_tabBar->setTabIcon(tabIndex, QIcon(":/images/application_double.png"));
    return tabIndex;
}

QLayout *ApplicationsPage::setupGroupHeader()
{
    auto layout = new QHBoxLayout();

    setupGroupOptions();

    layout->addWidget(m_btGroupOptions);
    layout->addStretch();

    return layout;
}

void ApplicationsPage::setupGroupOptions()
{
    m_btGroupOptions = new QPushButton();
    m_btGroupOptions->setIcon(QIcon(":/images/application_key.png"));

    setupGroupLimitIn();
    setupGroupLimitOut();
    setupGroupFragmentPacket();

    // Menu
    auto menu = new QMenu(m_btGroupOptions);

    auto waLimitIn = new QWidgetAction(this);
    waLimitIn->setDefaultWidget(m_cscLimitIn);
    menu->addAction(waLimitIn);

    auto waLimitOut = new QWidgetAction(this);
    waLimitOut->setDefaultWidget(m_cscLimitOut);
    menu->addAction(waLimitOut);

    menu->addSeparator();

    auto waFragmentPacket = new QWidgetAction(this);
    waFragmentPacket->setDefaultWidget(m_cbFragmentPacket);
    menu->addAction(waFragmentPacket);

    m_btGroupOptions->setMenu(menu);
}

void ApplicationsPage::setupGroupLimitIn()
{
    m_cscLimitIn = new CheckSpinCombo();
    m_cscLimitIn->spinBox()->setRange(0, 99999);
    m_cscLimitIn->setValues(speedLimitValues);

    connect(m_cscLimitIn->checkBox(), &QCheckBox::toggled, [&](bool checked) {
        if (appGroup()->limitInEnabled() == checked)
            return;

        appGroup()->setLimitInEnabled(checked);

        ctrl()->setConfEdited(true);
    });
    connect(m_cscLimitIn->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
        const auto speedLimit = quint32(value);

        if (appGroup()->speedLimitIn() == speedLimit)
            return;

        appGroup()->setSpeedLimitIn(speedLimit);

        ctrl()->setConfEdited(true);
    });
}

void ApplicationsPage::setupGroupLimitOut()
{
    m_cscLimitOut = new CheckSpinCombo();
    m_cscLimitOut->spinBox()->setRange(0, 99999);
    m_cscLimitOut->setValues(speedLimitValues);

    connect(m_cscLimitOut->checkBox(), &QCheckBox::toggled, [&](bool checked) {
        if (appGroup()->limitOutEnabled() == checked)
            return;

        appGroup()->setLimitOutEnabled(checked);

        ctrl()->setConfEdited(true);
    });
    connect(m_cscLimitOut->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
        const auto speedLimit = quint32(value);

        if (appGroup()->speedLimitOut() == speedLimit)
            return;

        appGroup()->setSpeedLimitOut(speedLimit);

        ctrl()->setConfEdited(true);
    });
}

void ApplicationsPage::setupGroupFragmentPacket()
{
    m_cbFragmentPacket = ControlUtil::createCheckBox(false, [&](bool checked) {
        if (appGroup()->fragmentPacket() == checked)
            return;

        appGroup()->setFragmentPacket(checked);

        ctrl()->setConfEdited(true);
    });
}

void ApplicationsPage::retranslateGroupLimits()
{
    QStringList list;

    list.append(tr("Custom"));
    list.append(tr("Disabled"));

    int index = 0;
    for (const int v : speedLimitValues) {
        if (++index > 2) {
            list.append(formatSpeed(v));
        }
    }

    m_cscLimitIn->setNames(list);
    m_cscLimitOut->setNames(list);
}

void ApplicationsPage::refreshGroup()
{
    m_cscLimitIn->checkBox()->setChecked(appGroup()->limitInEnabled());
    m_cscLimitIn->spinBox()->setValue(int(appGroup()->speedLimitIn()));

    m_cscLimitOut->checkBox()->setChecked(appGroup()->limitOutEnabled());
    m_cscLimitOut->spinBox()->setValue(int(appGroup()->speedLimitOut()));

    m_cbFragmentPacket->setChecked(appGroup()->fragmentPacket());
}

void ApplicationsPage::setupAppGroup()
{
    const auto refreshAppGroup = [&] {
        const int tabIndex = m_tabBar->currentIndex();
        m_appGroup = appGroupByIndex(tabIndex);

        refreshGroup();
    };

    refreshAppGroup();

    connect(m_tabBar, &QTabBar::currentChanged, this, refreshAppGroup);
}

int ApplicationsPage::appGroupsCount() const
{
    return conf()->appGroupsList().size();
}

AppGroup *ApplicationsPage::appGroupByIndex(int index) const
{
    return conf()->appGroupsList().at(index);
}

void ApplicationsPage::resetGroupName()
{
    m_editGroupName->setText(QString());
    m_editGroupName->setFocus();
}

QString ApplicationsPage::formatSpeed(int kbytes)
{
    return NetUtil::formatSpeed(quint32(kbytes * 1024));
}
