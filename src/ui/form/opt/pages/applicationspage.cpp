#include "applicationspage.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/appgroup.h>
#include <conf/firewallconf.h>
#include <form/controls/checkspincombo.h>
#include <form/controls/checktimeperiod.h>
#include <form/controls/controlutil.h>
#include <form/controls/labeldoublespin.h>
#include <form/controls/labelspin.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/tabbar.h>
#include <form/controls/textarea2splitter.h>
#include <form/controls/textarea2splitterhandle.h>
#include <form/dialog/dialogutil.h>
#include <form/opt/optionscontroller.h>
#include <fortsettings.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/net/netutil.h>
#include <util/textareautil.h>

#include "apps/appscolumn.h"

namespace {

const std::array speedLimitValues = { 10, 0, 20, 30, 50, 75, 100, 150, 200, 300, 500, 900, 1024,
    qRound(1.5 * 1024), 2 * 1024, 3 * 1024, 5 * 1024, qRound(7.5 * 1024), 10 * 1024, 15 * 1024,
    20 * 1024, 30 * 1024, 50 * 1024 };

CheckSpinCombo *createGroupLimit()
{
    auto c = new CheckSpinCombo();
    c->spinBox()->setRange(0, 99999);
    c->spinBox()->setSuffix(" KiB/s");
    c->setValues(speedLimitValues);
    return c;
}

QString formatSpeed(int kbytes)
{
    return NetUtil::formatSpeed(quint32(kbytes * 1024));
}

}

ApplicationsPage::ApplicationsPage(OptionsController *ctrl, QWidget *parent) :
    OptBasePage(ctrl, parent)
{
    setupUi();

    setupAppGroup();
}

AppGroup *ApplicationsPage::appGroup() const
{
    return appGroupByIndex(appGroupIndex());
}

void ApplicationsPage::setAppGroupIndex(int v)
{
    if (m_appGroupIndex != v) {
        m_appGroupIndex = v;
        emit appGroupChanged();
    }
}

void ApplicationsPage::onSaveWindowState(IniUser *ini)
{
    ini->setOptWindowAppsSplit(m_splitter->saveState());
}

void ApplicationsPage::onRestoreWindowState(IniUser *ini)
{
    m_splitter->restoreState(ini->optWindowAppsSplit());
}

void ApplicationsPage::onRetranslateUi()
{
    m_editGroupName->setPlaceholderText(tr("Group Name"));
    m_btAddGroup->setText(tr("Add Group"));
    m_btRenameGroup->setText(tr("Rename Group"));

    m_btGroupOptions->setText(tr("Tweaks"));
    m_cbApplyChild->setText(tr("Apply same rules to child processes"));
    m_cbLanOnly->setText(tr("Restrict access to LAN only"));

    m_cbLogBlocked->setText(tr("Collect blocked connections"));
    m_cbLogConn->setText(tr("Collect connection statistics"));

    m_cscLimitIn->checkBox()->setText(tr("Download speed limit:"));
    m_cscLimitOut->checkBox()->setText(tr("Upload speed limit:"));
    retranslateGroupLimits();

    m_limitLatency->label()->setText(tr("Latency:"));
    m_limitPacketLoss->label()->setText(tr("Packet Loss:"));
    m_limitBufferSizeIn->label()->setText(tr("Download Buffer Size:"));
    m_limitBufferSizeOut->label()->setText(tr("Upload Buffer Size:"));

    m_cbGroupEnabled->setText(tr("Enabled"));
    m_ctpGroupPeriod->checkBox()->setText(tr("time period:"));

    m_blockApps->labelTitle()->setText(tr("Block"));
    m_allowApps->labelTitle()->setText(tr("Allow"));

    auto splitterHandle = m_splitter->handle();
    splitterHandle->btMoveAllFrom1To2()->setToolTip(tr("Move All Lines to 'Allow'"));
    splitterHandle->btMoveAllFrom2To1()->setToolTip(tr("Move All Lines to 'Block'"));
    splitterHandle->btInterchangeAll()->setToolTip(tr("Interchange All Lines"));
    splitterHandle->btMoveSelectedFrom1To2()->setToolTip(tr("Move Selected Lines to 'Allow'"));
    splitterHandle->btMoveSelectedFrom2To1()->setToolTip(tr("Move Selected Lines to 'Block'"));
    m_btSelectFile->setToolTip(tr("Select File"));

    retranslateAppsPlaceholderText();
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

void ApplicationsPage::retranslateAppsPlaceholderText()
{
    const auto placeholderText = tr("# Examples:") + '\n'
            + QLatin1String("System\n"
                            "C:\\Program Files (x86)\\Microsoft\\Skype for Desktop\\Skype.exe\n"
                            "%SystemRoot%\\System32\\telnet.exe\n")
            + '\n' + tr("# All programs in the sub-path:") + QLatin1String("\nC:\\Git\\**");

    m_allowApps->editText()->setPlaceholderText(placeholderText);
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

    // App Columns
    setupBlockApps();
    setupAllowApps();

    // Splitter
    setupSplitter();
    layout->addWidget(m_splitter, 1);

    // Splitter Buttons
    setupSplitterButtons();

    this->setLayout(layout);
}

QLayout *ApplicationsPage::setupHeader()
{
    auto layout = new QHBoxLayout();

    m_editGroupName = new QLineEdit();
    m_editGroupName->setClearButtonEnabled(true);
    m_editGroupName->setMaxLength(128);
    m_editGroupName->setFixedWidth(200);

    setupAddGroup();
    setupRenameGroup();

    layout->addWidget(m_editGroupName);
    layout->addWidget(m_btAddGroup);
    layout->addWidget(m_btRenameGroup);
    layout->addStretch();

    return layout;
}

void ApplicationsPage::setupAddGroup()
{
    m_btAddGroup = ControlUtil::createButton(":/icons/add.png", [&] {
        const auto text = m_editGroupName->text();
        if (text.isEmpty()) {
            m_editGroupName->setFocus();
            return;
        }

        conf()->addAppGroupByName(text);

        const int tabIndex = m_tabBar->addTab(text);
        m_tabBar->setCurrentIndex(tabIndex);

        resetGroupName();

        ctrl()->setOptEdited();
        ctrl()->setFlagsEdited(); // to adjust appGroupBits
    });

    const auto refreshAddGroup = [&] {
        m_btAddGroup->setEnabled(appGroupsCount() < MAX_APP_GROUP_COUNT);
    };

    refreshAddGroup();

    connect(conf(), &FirewallConf::appGroupsChanged, this, refreshAddGroup);
}

void ApplicationsPage::setupRenameGroup()
{
    m_btRenameGroup = ControlUtil::createButton(":/icons/pencil.png", [&] {
        const auto text = m_editGroupName->text();
        if (text.isEmpty()) {
            m_editGroupName->setFocus();
            return;
        }

        const int tabIndex = m_tabBar->currentIndex();
        m_tabBar->setTabText(tabIndex, text);

        AppGroup *appGroup = this->appGroup();
        appGroup->setName(text);

        resetGroupName();

        ctrl()->setOptEdited();
    });
}

void ApplicationsPage::setupTabBar()
{
    m_tabBar = new TabBar();
    m_tabBar->setTabMinimumWidth(100);
    m_tabBar->setShape(QTabBar::TriangularNorth);
    m_tabBar->setExpanding(false);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);

    for (const auto appGroup : appGroups()) {
        addTab(appGroup->name());
    }

    connect(m_tabBar, &QTabBar::tabCloseRequested, this, [&](int index) {
        if (m_tabBar->count() <= 1)
            return;

        conf()->removeAppGroup(index, index);

        const int tabIndex = m_tabBar->currentIndex();
        m_tabBar->removeTab(index);
        if (tabIndex == m_tabBar->currentIndex()) {
            emit appGroupChanged();
        }

        ctrl()->setOptEdited();
    });
    connect(m_tabBar, &QTabBar::tabMoved, this, [&](int from, int to) {
        conf()->moveAppGroup(from, to);
        ctrl()->setOptEdited();
    });
}

int ApplicationsPage::addTab(const QString &text)
{
    const int tabIndex = m_tabBar->addTab(text);
    return tabIndex;
}

QLayout *ApplicationsPage::setupGroupHeader()
{
    auto layout = new QHBoxLayout();

    setupGroupEnabled();
    setupGroupPeriod();
    setupGroupPeriodEnabled();
    setupGroupOptions();

    layout->addWidget(m_cbGroupEnabled);
    layout->addWidget(m_ctpGroupPeriod);
    layout->addStretch(1);
    layout->addWidget(m_btGroupOptions);

    return layout;
}

void ApplicationsPage::setupGroupEnabled()
{
    m_cbGroupEnabled = ControlUtil::createCheckBox(false, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->enabled() == checked)
            return;

        appGroup->setEnabled(checked);

        ctrl()->setFlagsEdited();
    });

    m_cbGroupEnabled->setFont(ControlUtil::fontDemiBold());
}

void ApplicationsPage::setupGroupPeriod()
{
    m_ctpGroupPeriod = new CheckTimePeriod();

    connect(m_ctpGroupPeriod->checkBox(), &QCheckBox::toggled, this, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->periodEnabled() == checked)
            return;

        appGroup->setPeriodEnabled(checked);

        ctrl()->setOptEdited();
    });
    connect(m_ctpGroupPeriod->timeEdit1(), &QTimeEdit::userTimeChanged, this,
            [&](const QTime &time) {
                const auto timeStr = CheckTimePeriod::fromTime(time);

                AppGroup *appGroup = this->appGroup();
                if (appGroup->periodFrom() == timeStr)
                    return;

                appGroup->setPeriodFrom(timeStr);

                ctrl()->setOptEdited();
            });
    connect(m_ctpGroupPeriod->timeEdit2(), &QTimeEdit::userTimeChanged, this,
            [&](const QTime &time) {
                const auto timeStr = CheckTimePeriod::fromTime(time);

                AppGroup *appGroup = this->appGroup();
                if (appGroup->periodTo() == timeStr)
                    return;

                appGroup->setPeriodTo(timeStr);

                ctrl()->setOptEdited();
            });
}

void ApplicationsPage::setupGroupPeriodEnabled()
{
    const auto refreshPeriodEnabled = [&] {
        m_ctpGroupPeriod->setEnabled(m_cbGroupEnabled->isChecked());
    };

    refreshPeriodEnabled();

    connect(m_cbGroupEnabled, &QCheckBox::toggled, this, refreshPeriodEnabled);
}

void ApplicationsPage::setupGroupOptions()
{
    setupGroupOptionFlags();
    setupGroupLog();
    setupGroupLimitIn();
    setupGroupLimitOut();
    setupGroupLimitLatency();
    setupGroupLimitPacketLoss();
    setupGroupLimitBufferSize();

    // Menu
    const QList<QWidget *> menuWidgets = { m_cbApplyChild, m_cbLanOnly,
        ControlUtil::createSeparator(), m_cbLogBlocked, m_cbLogConn, ControlUtil::createSeparator(),
        m_cscLimitIn, m_cscLimitOut, m_limitLatency, m_limitPacketLoss, m_limitBufferSizeIn,
        m_limitBufferSizeOut };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btGroupOptions = ControlUtil::createButton(":/icons/wrench.png");
    m_btGroupOptions->setMenu(menu);
}

void ApplicationsPage::setupGroupOptionFlags()
{
    m_cbApplyChild = ControlUtil::createCheckBox(false, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->applyChild() == checked)
            return;

        appGroup->setApplyChild(checked);

        ctrl()->setOptEdited();
    });

    m_cbLanOnly = ControlUtil::createCheckBox(false, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->lanOnly() == checked)
            return;

        appGroup->setLanOnly(checked);

        ctrl()->setOptEdited();
    });
}

void ApplicationsPage::setupGroupLog()
{
    m_cbLogBlocked = ControlUtil::createCheckBox(false, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->logBlocked() == checked)
            return;

        appGroup->setLogBlocked(checked);

        ctrl()->setOptEdited();
    });

    m_cbLogConn = ControlUtil::createCheckBox(false, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->logConn() == checked)
            return;

        appGroup->setLogConn(checked);

        ctrl()->setOptEdited();
    });

    m_cbLogConn->setVisible(false); // TODO: Collect allowed connections
}

void ApplicationsPage::setupGroupLimitIn()
{
    m_cscLimitIn = createGroupLimit();

    connect(m_cscLimitIn->checkBox(), &QCheckBox::toggled, this, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->limitInEnabled() == checked)
            return;

        appGroup->setLimitInEnabled(checked);

        ctrl()->setOptEdited();
    });
    connect(m_cscLimitIn->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                const auto kbytes = quint32(value);

                AppGroup *appGroup = this->appGroup();
                if (appGroup->speedLimitIn() == kbytes)
                    return;

                appGroup->setSpeedLimitIn(kbytes);

                ctrl()->setOptEdited();
            });
}

void ApplicationsPage::setupGroupLimitOut()
{
    m_cscLimitOut = createGroupLimit();

    connect(m_cscLimitOut->checkBox(), &QCheckBox::toggled, this, [&](bool checked) {
        AppGroup *appGroup = this->appGroup();
        if (appGroup->limitOutEnabled() == checked)
            return;

        appGroup->setLimitOutEnabled(checked);

        ctrl()->setOptEdited();
    });
    connect(m_cscLimitOut->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                const auto kbytes = quint32(value);

                AppGroup *appGroup = this->appGroup();
                if (appGroup->speedLimitOut() == kbytes)
                    return;

                appGroup->setSpeedLimitOut(kbytes);

                ctrl()->setOptEdited();
            });
}

void ApplicationsPage::setupGroupLimitLatency()
{
    m_limitLatency = ControlUtil::createSpin(0, 0, 30000, " ms", [&](int value) {
        const auto limitLatency = quint32(value);

        AppGroup *appGroup = this->appGroup();
        if (appGroup->limitLatency() == limitLatency)
            return;

        appGroup->setLimitLatency(limitLatency);

        ctrl()->setOptEdited();
    });
}

void ApplicationsPage::setupGroupLimitPacketLoss()
{
    m_limitPacketLoss = ControlUtil::createDoubleSpin(0, 0, 100.0, " %", [&](double value) {
        const auto limitPacketLoss = quint16(qFloor(value * 100.0));

        AppGroup *appGroup = this->appGroup();
        if (appGroup->limitPacketLoss() == limitPacketLoss)
            return;

        appGroup->setLimitPacketLoss(limitPacketLoss);

        ctrl()->setOptEdited();
    });
}

void ApplicationsPage::setupGroupLimitBufferSize()
{
    constexpr int maxBufferSize = 2 * 1024 * 1024;
    const QLatin1String suffix(" bytes");

    m_limitBufferSizeIn = ControlUtil::createSpin(0, 0, maxBufferSize, suffix, [&](int value) {
        const auto bufferSize = quint32(value);

        AppGroup *appGroup = this->appGroup();
        if (appGroup->limitBufferSizeIn() == bufferSize)
            return;

        appGroup->setLimitBufferSizeIn(bufferSize);

        ctrl()->setOptEdited();
    });

    m_limitBufferSizeOut = ControlUtil::createSpin(0, 0, maxBufferSize, suffix, [&](int value) {
        const auto bufferSize = quint32(value);

        AppGroup *appGroup = this->appGroup();
        if (appGroup->limitBufferSizeOut() == bufferSize)
            return;

        appGroup->setLimitBufferSizeOut(bufferSize);

        ctrl()->setOptEdited();
    });
}

void ApplicationsPage::setupBlockApps()
{
    m_blockApps = new AppsColumn();
    m_blockApps->icon()->setPixmap(IconCache::file(":/icons/deny.png"));

    connect(m_blockApps->editText(), &QPlainTextEdit::textChanged, this, [&] {
        const auto text = m_blockApps->editText()->toPlainText();

        AppGroup *appGroup = this->appGroup();
        if (appGroup->blockText() == text)
            return;

        appGroup->setBlockText(text);

        ctrl()->setOptEdited();
    });
}

void ApplicationsPage::setupAllowApps()
{
    m_allowApps = new AppsColumn();
    m_allowApps->icon()->setPixmap(IconCache::file(":/icons/accept.png"));

    connect(m_allowApps->editText(), &QPlainTextEdit::textChanged, this, [&] {
        const auto text = m_allowApps->editText()->toPlainText();

        AppGroup *appGroup = this->appGroup();
        if (appGroup->allowText() == text)
            return;

        appGroup->setAllowText(text);

        ctrl()->setOptEdited();
    });
}

void ApplicationsPage::setupSplitter()
{
    m_splitter = new TextArea2Splitter();

    Q_ASSERT(!m_splitter->handle());

    m_splitter->addWidget(m_blockApps);
    m_splitter->addWidget(m_allowApps);

    auto splitterHandle = m_splitter->handle();
    Q_ASSERT(splitterHandle);

    splitterHandle->setTextArea1(m_blockApps->editText());
    splitterHandle->setTextArea2(m_allowApps->editText());
}

void ApplicationsPage::setupSplitterButtons()
{
    m_btSelectFile = ControlUtil::createSplitterButton(":/icons/folder.png", [&] {
        const auto filePaths = DialogUtil::getOpenFileNames(
                m_btSelectFile->text(), tr("Programs (*.exe);;All files (*.*)"));

        if (!filePaths.isEmpty()) {
            auto area = m_splitter->handle()->currentTextArea();
            TextAreaUtil::appendText(area, filePaths.join('\n'));
        }
    });

    const auto layout = m_splitter->handle()->buttonsLayout();
    layout->addWidget(m_btSelectFile, 0, Qt::AlignHCenter);
}

void ApplicationsPage::updateGroup()
{
    AppGroup *appGroup = this->appGroup();

    m_cbApplyChild->setChecked(appGroup->applyChild());
    m_cbLanOnly->setChecked(appGroup->lanOnly());

    m_cbLogBlocked->setChecked(appGroup->logBlocked());
    m_cbLogConn->setChecked(appGroup->logConn());

    m_cscLimitIn->checkBox()->setChecked(appGroup->limitInEnabled());
    m_cscLimitIn->spinBox()->setValue(int(appGroup->speedLimitIn()));

    m_cscLimitOut->checkBox()->setChecked(appGroup->limitOutEnabled());
    m_cscLimitOut->spinBox()->setValue(int(appGroup->speedLimitOut()));

    m_limitLatency->spinBox()->setValue(int(appGroup->limitLatency()));
    m_limitPacketLoss->spinBox()->setValue(double(appGroup->limitPacketLoss()) / 100.0);
    m_limitBufferSizeIn->spinBox()->setValue(int(appGroup->limitBufferSizeIn()));
    m_limitBufferSizeOut->spinBox()->setValue(int(appGroup->limitBufferSizeOut()));

    m_cbGroupEnabled->setChecked(appGroup->enabled());

    m_ctpGroupPeriod->checkBox()->setChecked(appGroup->periodEnabled());
    m_ctpGroupPeriod->timeEdit1()->setTime(CheckTimePeriod::toTime(appGroup->periodFrom()));
    m_ctpGroupPeriod->timeEdit2()->setTime(CheckTimePeriod::toTime(appGroup->periodTo()));

    m_blockApps->editText()->setText(appGroup->blockText());
    m_allowApps->editText()->setText(appGroup->allowText());
}

void ApplicationsPage::setupAppGroup()
{
    connect(this, &ApplicationsPage::appGroupChanged, this, &ApplicationsPage::updateGroup);

    const auto refreshAppGroup = [&](int tabIndex) { setAppGroupIndex(tabIndex); };

    refreshAppGroup(m_tabBar->currentIndex());

    connect(m_tabBar, &QTabBar::currentChanged, this, refreshAppGroup);
}

const QList<AppGroup *> &ApplicationsPage::appGroups() const
{
    return conf()->appGroups();
}

int ApplicationsPage::appGroupsCount() const
{
    return appGroups().size();
}

AppGroup *ApplicationsPage::appGroupByIndex(int index) const
{
    return appGroups().at(index);
}

void ApplicationsPage::resetGroupName()
{
    m_editGroupName->clear();
    m_editGroupName->setFocus();
}
