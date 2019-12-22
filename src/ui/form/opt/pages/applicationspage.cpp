#include "applicationspage.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../conf/appgroup.h"
#include "../../../conf/firewallconf.h"
#include "../../controls/controlutil.h"
#include "../../controls/tabbar.h"
#include "../optionscontroller.h"

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

        appGroup(tabIndex)->setName(text);
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
            m_tabBar->setTabText(0, appGroup(0)->name());
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

int ApplicationsPage::appGroupsCount() const
{
    return conf()->appGroupsList().size();
}

AppGroup *ApplicationsPage::appGroup(int tabIndex) const
{
    return conf()->appGroupsList().at(tabIndex);
}

void ApplicationsPage::resetGroupName()
{
    m_editGroupName->setText(QString());
    m_editGroupName->setFocus();
}
