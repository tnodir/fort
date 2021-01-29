#include "statisticspage.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabBar>
#include <QTableView>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "../../../conf/firewallconf.h"
#include "../../../fortmanager.h"
#include "../../../fortsettings.h"
#include "../../../log/logmanager.h"
#include "../../../model/appstatmodel.h"
#include "../../../model/traflistmodel.h"
#include "../../../util/app/appinfocache.h"
#include "../../../util/guiutil.h"
#include "../../../util/net/netutil.h"
#include "../../../util/osutil.h"
#include "../../controls/checktimeperiod.h"
#include "../../controls/controlutil.h"
#include "../../controls/labelcolor.h"
#include "../../controls/labelspin.h"
#include "../../controls/labelspincombo.h"
#include "../../controls/listview.h"
#include "../optionscontroller.h"

namespace {

const ValuesList trafKeepDayValues = { 60, -1, 90, 180, 365, 365 * 3 };
const ValuesList trafKeepMonthValues = { 2, -1, 3, 6, 12, 36 };
const ValuesList logIpKeepCountValues = { 1000, -1, 1000, 5000, 10000, 50000, 100000, 500000,
    1000000, 5000000, 10000000 };
const ValuesList quotaValues = { 10, 0, 100, 500, 1024, 8 * 1024, 10 * 1024, 30 * 1024, 50 * 1024,
    100 * 1024 };

}

StatisticsPage::StatisticsPage(OptionsController *ctrl, QWidget *parent) :
    BasePage(ctrl, parent), m_graphEdited(false), m_pageUpdating(false)
{
    setupTrafListModel();

    setupUi();
    updatePage();
}

void StatisticsPage::setGraphEdited(bool v)
{
    if (m_graphEdited != v) {
        m_graphEdited = v;

        if (graphEdited()) {
            ctrl()->setOthersEdited(true);
        }
    }
}

AppStatModel *StatisticsPage::appStatModel() const
{
    return fortManager()->logManager()->appStatModel();
}

AppInfoCache *StatisticsPage::appInfoCache() const
{
    return appStatModel()->appInfoCache();
}

void StatisticsPage::onEditResetted()
{
    setGraphEdited(false);
}

void StatisticsPage::onSaved()
{
    if (!graphEdited())
        return;

    settings()->setGraphWindowAlwaysOnTop(m_cbGraphAlwaysOnTop->isChecked());
    settings()->setGraphWindowFrameless(m_cbGraphFrameless->isChecked());
    settings()->setGraphWindowClickThrough(m_cbGraphClickThrough->isChecked());
    settings()->setGraphWindowHideOnHover(m_cbGraphHideOnHover->isChecked());

    settings()->setGraphWindowOpacity(m_graphOpacity->spinBox()->value());
    settings()->setGraphWindowHoverOpacity(m_graphHoverOpacity->spinBox()->value());
    settings()->setGraphWindowMaxSeconds(m_graphMaxSeconds->spinBox()->value());

    settings()->setGraphWindowColor(m_graphColor->color());
    settings()->setGraphWindowColorIn(m_graphColorIn->color());
    settings()->setGraphWindowColorOut(m_graphColorOut->color());
    settings()->setGraphWindowAxisColor(m_graphAxisColor->color());
    settings()->setGraphWindowTickLabelColor(m_graphTickLabelColor->color());
    settings()->setGraphWindowLabelColor(m_graphLabelColor->color());
    settings()->setGraphWindowGridColor(m_graphGridColor->color());

    fortManager()->updateGraphWindow();
}

void StatisticsPage::onSaveWindowState()
{
    settings()->setOptWindowStatSplit(m_splitter->saveState());
}

void StatisticsPage::onRestoreWindowState()
{
    m_splitter->restoreState(settings()->optWindowStatSplit());
}

void StatisticsPage::onRetranslateUi()
{
    m_btRefresh->setText(tr("Refresh"));
    m_btClear->setText(tr("Clear"));

    m_actRemoveApp->setText(tr("Remove Application"));
    m_actResetTotal->setText(tr("Reset Total"));
    m_actClearAll->setText(tr("Clear All"));

    m_btTrafOptions->setText(tr("Options"));
    m_cbLogStat->setText(tr("Collect Traffic Statistics"));
    m_ctpActivePeriod->checkBox()->setText(tr("Active time period:"));
    m_lscMonthStart->label()->setText(tr("Month starts on:"));

    m_lscTrafHourKeepDays->label()->setText(tr("Keep data for 'Hourly':"));
    m_lscTrafHourKeepDays->spinBox()->setSuffix(tr(" day(s)"));
    m_lscTrafDayKeepDays->label()->setText(tr("Keep data for 'Daily':"));
    m_lscTrafDayKeepDays->spinBox()->setSuffix(tr(" day(s)"));
    m_lscTrafMonthKeepMonths->label()->setText(tr("Keep data for 'Monthly':"));
    m_lscTrafMonthKeepMonths->spinBox()->setSuffix(tr(" month(s)"));

    m_cbLogAllowedIp->setText(tr("Collect connection statistics"));
    m_lscAllowedIpKeepCount->label()->setText(tr("Keep count for 'Allowed connections':"));
    m_lscBlockedIpKeepCount->label()->setText(tr("Keep count for 'Blocked connections':"));

    m_lscQuotaDayMb->label()->setText(tr("Day's Quota:"));
    m_lscQuotaMonthMb->label()->setText(tr("Month's Quota:"));

    retranslateTrafKeepDayNames();
    retranslateTrafKeepMonthNames();
    retranslateIpKeepCountNames();
    retranslateQuotaNames();

    m_btGraphOptions->setText(tr("Graph"));
    m_cbGraphAlwaysOnTop->setText(tr("Always on top"));
    m_cbGraphFrameless->setText(tr("Frameless"));
    m_cbGraphClickThrough->setText(tr("Click through"));
    m_cbGraphHideOnHover->setText(tr("Hide on hover"));
    m_graphOpacity->label()->setText(tr("Opacity:"));
    m_graphHoverOpacity->label()->setText(tr("Hover opacity:"));
    m_graphMaxSeconds->label()->setText(tr("Max seconds:"));
    m_graphColor->label()->setText(tr("Background:"));
    m_graphColorIn->label()->setText(tr("Download:"));
    m_graphColorOut->label()->setText(tr("Upload:"));
    m_graphAxisColor->label()->setText(tr("Axis:"));
    m_graphTickLabelColor->label()->setText(tr("Tick label:"));
    m_graphLabelColor->label()->setText(tr("Label:"));
    m_graphGridColor->label()->setText(tr("Grid:"));

    m_traphUnits->setText(tr("Units:"));
    retranslateTrafUnitNames();

    retranslateTabBar();

    m_btAppCopyPath->setToolTip(tr("Copy Path"));
    m_btAppOpenFolder->setToolTip(tr("Open Folder"));
}

void StatisticsPage::retranslateTrafKeepDayNames()
{
    const QStringList list = { tr("Custom"), tr("Forever"), tr("3 months"), tr("6 months"),
        tr("1 year"), tr("3 years") };

    m_lscTrafHourKeepDays->setNames(list);
    m_lscTrafDayKeepDays->setNames(list);
}

void StatisticsPage::retranslateTrafKeepMonthNames()
{
    const QStringList list = { tr("Custom"), tr("Forever"), tr("3 months"), tr("6 months"),
        tr("1 year"), tr("3 years") };

    m_lscTrafMonthKeepMonths->setNames(list);
}

void StatisticsPage::retranslateIpKeepCountNames()
{
    const QStringList list = { tr("Custom"), tr("Forever"), "1K", "5K", "10K", "50K", "100K",
        "500K", "1M", "5M", "10M" };

    m_lscAllowedIpKeepCount->setNames(list);
    m_lscBlockedIpKeepCount->setNames(list);
}

void StatisticsPage::retranslateQuotaNames()
{
    QStringList list;

    list.append(tr("Custom"));
    list.append(tr("Disabled"));

    int index = 0;
    for (const int v : quotaValues) {
        if (++index > 2) {
            list.append(formatQuota(v));
        }
    }

    m_lscQuotaDayMb->setNames(list);
    m_lscQuotaMonthMb->setNames(list);
}

void StatisticsPage::retranslateTrafUnitNames()
{
    const QStringList list = { tr("Adaptive"), tr("Bytes"), "KiB", "MiB", "GiB", "TiB" };

    m_comboTrafUnit->clear();
    m_comboTrafUnit->addItems(list);

    updateTrafUnit();
}

void StatisticsPage::retranslateTabBar()
{
    const QStringList list = { tr("Hourly"), tr("Daily"), tr("Monthly"), tr("Total") };

    int index = 0;
    for (const auto &v : list) {
        m_tabBar->setTabText(index++, v);
    }
}

void StatisticsPage::setupTrafListModel()
{
    m_trafListModel = appStatModel()->trafListModel();
}

void StatisticsPage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Content
    m_splitter = new QSplitter();

    setupAppListView();
    m_splitter->addWidget(m_appListView);

    // Tab Bar
    auto trafLayout = new QVBoxLayout();
    trafLayout->setContentsMargins(0, 0, 0, 0);

    setupTabBar();
    trafLayout->addWidget(m_tabBar);

    // Traf Table
    setupTableTraf();
    setupTableTrafHeader();
    trafLayout->addWidget(m_tableTraf);

    auto trafWidget = new QWidget();
    trafWidget->setLayout(trafLayout);
    m_splitter->addWidget(trafWidget);

    layout->addWidget(m_splitter, 1);

    // App Info Row
    setupAppInfoRow();
    setupAppInfoVersion();
    layout->addWidget(m_appInfoRow);

    // Actions on app list view's current changed
    setupAppListViewChanged();

    this->setLayout(layout);
}

QLayout *StatisticsPage::setupHeader()
{
    auto layout = new QHBoxLayout();

    m_btRefresh =
            ControlUtil::createButton(":/icons/sign-sync.png", [&] { trafListModel()->reset(); });

    setupClearMenu();
    setupTrafUnits();
    setupGraphOptionsMenu();
    setupTrafOptionsMenu();

    layout->addWidget(m_btRefresh);
    layout->addWidget(m_btClear);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_traphUnits);
    layout->addWidget(m_comboTrafUnit);
    layout->addStretch();
    layout->addWidget(m_btGraphOptions);
    layout->addWidget(m_btTrafOptions);

    return layout;
}

void StatisticsPage::setupClearMenu()
{
    auto menu = new QMenu(this);

    m_actRemoveApp = menu->addAction(QIcon(":/icons/sign-delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    m_actResetTotal = menu->addAction(QString());
    m_actClearAll = menu->addAction(QString());

    connect(m_actRemoveApp, &QAction::triggered, this, [&] {
        if (!fortManager()->showQuestionBox(
                    tr("Are you sure to remove statistics for selected application?")))
            return;

        appStatModel()->remove(appListCurrentIndex());
    });
    connect(m_actResetTotal, &QAction::triggered, this, [&] {
        if (!fortManager()->showQuestionBox(tr("Are you sure to reset total statistics?")))
            return;

        trafListModel()->resetAppTotals();
    });
    connect(m_actClearAll, &QAction::triggered, this, [&] {
        if (!fortManager()->showQuestionBox(tr("Are you sure to clear all statistics?")))
            return;

        m_appListView->clearSelection();
        appStatModel()->clear();
    });

    m_btClear = new QPushButton(QIcon(":/icons/trash.png"), QString());
    m_btClear->setMenu(menu);
}

void StatisticsPage::setupTrafUnits()
{
    m_traphUnits = ControlUtil::createLabel();

    m_comboTrafUnit = ControlUtil::createComboBox(QStringList(), [&](int index) {
        if (conf()->trafUnit() == index)
            return;

        conf()->setTrafUnit(index);

        fortManager()->applyConfImmediateFlags();
    });
}

void StatisticsPage::setupGraphOptionsMenu()
{
    m_cbGraphAlwaysOnTop = new QCheckBox();
    m_cbGraphFrameless = new QCheckBox();
    m_cbGraphClickThrough = new QCheckBox();
    m_cbGraphHideOnHover = new QCheckBox();

    m_graphOpacity = createSpin(0, 100, " %");
    m_graphHoverOpacity = createSpin(0, 100, " %");
    m_graphMaxSeconds = createSpin(0, 9999);

    m_graphColor = new LabelColor();
    m_graphColorIn = new LabelColor();
    m_graphColorOut = new LabelColor();
    m_graphAxisColor = new LabelColor();
    m_graphTickLabelColor = new LabelColor();
    m_graphLabelColor = new LabelColor();
    m_graphGridColor = new LabelColor();

    const auto onChanged = [&] {
        if (!m_pageUpdating) {
            setGraphEdited(true);
        }
    };

    connect(m_cbGraphAlwaysOnTop, &QCheckBox::toggled, onChanged);
    connect(m_cbGraphFrameless, &QCheckBox::toggled, onChanged);
    connect(m_cbGraphClickThrough, &QCheckBox::toggled, onChanged);
    connect(m_cbGraphHideOnHover, &QCheckBox::toggled, onChanged);

    connect(m_graphOpacity->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), onChanged);
    connect(m_graphHoverOpacity->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), onChanged);
    connect(m_graphMaxSeconds->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), onChanged);

    connect(m_graphColor, &LabelColor::colorChanged, onChanged);
    connect(m_graphColorIn, &LabelColor::colorChanged, onChanged);
    connect(m_graphColorOut, &LabelColor::colorChanged, onChanged);
    connect(m_graphAxisColor, &LabelColor::colorChanged, onChanged);
    connect(m_graphTickLabelColor, &LabelColor::colorChanged, onChanged);
    connect(m_graphLabelColor, &LabelColor::colorChanged, onChanged);
    connect(m_graphGridColor, &LabelColor::colorChanged, onChanged);

    // Menu
    auto colLayout1 = ControlUtil::createLayoutByWidgets({ m_cbGraphAlwaysOnTop, m_cbGraphFrameless,
            m_cbGraphClickThrough, m_cbGraphHideOnHover, ControlUtil::createSeparator(),
            m_graphOpacity, m_graphHoverOpacity, m_graphMaxSeconds, nullptr });
    auto colLayout2 =
            ControlUtil::createLayoutByWidgets({ m_graphColor, m_graphColorIn, m_graphColorOut,
                    m_graphAxisColor, m_graphTickLabelColor, m_graphLabelColor, m_graphGridColor });
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addLayout(colLayout2);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btGraphOptions = new QPushButton(QIcon(":/icons/line-graph.png"), QString());
    m_btGraphOptions->setMenu(menu);
}

void StatisticsPage::setupTrafOptionsMenu()
{
    setupLogStat();
    setupActivePeriod();
    setupMonthStart();
    setupTrafHourKeepDays();
    setupTrafDayKeepDays();
    setupTrafMonthKeepMonths();
    setupLogAllowedIp();
    setupAllowedIpKeepCount();
    setupBlockedIpKeepCount();
    setupQuotaDayMb();
    setupQuotaMonthMb();

    // Menu
    const QList<QWidget *> menuWidgets = { m_cbLogStat, m_ctpActivePeriod, m_lscMonthStart,
        ControlUtil::createSeparator(), m_lscTrafHourKeepDays, m_lscTrafDayKeepDays,
        m_lscTrafMonthKeepMonths, ControlUtil::createSeparator(), m_cbLogAllowedIp,
        m_lscAllowedIpKeepCount, m_lscBlockedIpKeepCount, ControlUtil::createSeparator(),
        m_lscQuotaDayMb, m_lscQuotaMonthMb };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btTrafOptions = new QPushButton(QIcon(":/icons/wrench.png"), QString());
    m_btTrafOptions->setMenu(menu);
}

void StatisticsPage::setupLogStat()
{
    m_cbLogStat = ControlUtil::createCheckBox(false, [&](bool checked) {
        if (conf()->logStat() == checked)
            return;

        conf()->setLogStat(checked);

        fortManager()->applyConfImmediateFlags();
    });

    m_cbLogStat->setFont(ControlUtil::fontDemiBold());
}

void StatisticsPage::setupActivePeriod()
{
    m_ctpActivePeriod = new CheckTimePeriod();

    connect(m_ctpActivePeriod->checkBox(), &QCheckBox::toggled, this, [&](bool checked) {
        if (conf()->activePeriodEnabled() == checked)
            return;

        conf()->setActivePeriodEnabled(checked);

        ctrl()->setConfFlagsEdited(true);
    });
    connect(m_ctpActivePeriod->timeEdit1(), &QTimeEdit::userTimeChanged, this,
            [&](const QTime &time) {
                const auto timeStr = CheckTimePeriod::fromTime(time);

                if (conf()->activePeriodFrom() == timeStr)
                    return;

                conf()->setActivePeriodFrom(timeStr);

                ctrl()->setConfFlagsEdited(true);
            });
    connect(m_ctpActivePeriod->timeEdit2(), &QTimeEdit::userTimeChanged, this,
            [&](const QTime &time) {
                const auto timeStr = CheckTimePeriod::fromTime(time);

                if (conf()->activePeriodTo() == timeStr)
                    return;

                conf()->setActivePeriodTo(timeStr);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupMonthStart()
{
    m_lscMonthStart = createSpinCombo(1, 31);

    // Days list
    {
        ValuesList dayValues;
        dayValues.reserve(31);
        for (int i = 1; i <= 31; ++i) {
            dayValues.append(i);
        }
        m_lscMonthStart->setValues(dayValues);
        m_lscMonthStart->setNamesByValues();
    }

    connect(m_lscMonthStart->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (conf()->monthStart() == value)
                    return;

                conf()->setMonthStart(value);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupTrafHourKeepDays()
{
    m_lscTrafHourKeepDays = createSpinCombo(-1, 9999);
    m_lscTrafHourKeepDays->setValues(trafKeepDayValues);

    connect(m_lscTrafHourKeepDays->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (conf()->trafHourKeepDays() == value)
                    return;

                conf()->setTrafHourKeepDays(value);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupTrafDayKeepDays()
{
    m_lscTrafDayKeepDays = createSpinCombo(-1, 9999);
    m_lscTrafDayKeepDays->setValues(trafKeepDayValues);

    connect(m_lscTrafDayKeepDays->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (conf()->trafDayKeepDays() == value)
                    return;

                conf()->setTrafDayKeepDays(value);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupTrafMonthKeepMonths()
{
    m_lscTrafMonthKeepMonths = createSpinCombo(-1, 9999);
    m_lscTrafMonthKeepMonths->setValues(trafKeepMonthValues);

    connect(m_lscTrafMonthKeepMonths->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (conf()->trafMonthKeepMonths() == value)
                    return;

                conf()->setTrafMonthKeepMonths(value);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupLogAllowedIp()
{
    m_cbLogAllowedIp = ControlUtil::createCheckBox(conf()->logAllowedIp(), [&](bool checked) {
        if (conf()->logAllowedIp() == checked)
            return;

        conf()->setLogAllowedIp(checked);

        fortManager()->applyConfImmediateFlags();
    });
}

void StatisticsPage::setupAllowedIpKeepCount()
{
    m_lscAllowedIpKeepCount = new LabelSpinCombo();
    m_lscAllowedIpKeepCount->spinBox()->setRange(-1, 999999999);
    m_lscAllowedIpKeepCount->setValues(logIpKeepCountValues);

    connect(m_lscAllowedIpKeepCount->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (conf()->allowedIpKeepCount() == value)
                    return;

                conf()->setAllowedIpKeepCount(value);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupBlockedIpKeepCount()
{
    m_lscBlockedIpKeepCount = new LabelSpinCombo();
    m_lscBlockedIpKeepCount->spinBox()->setRange(-1, 999999999);
    m_lscBlockedIpKeepCount->setValues(logIpKeepCountValues);

    connect(m_lscBlockedIpKeepCount->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                if (conf()->blockedIpKeepCount() == value)
                    return;

                conf()->setBlockedIpKeepCount(value);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupQuotaDayMb()
{
    m_lscQuotaDayMb = createSpinCombo(0, 1024 * 1024, " MiB");
    m_lscQuotaDayMb->setValues(quotaValues);

    connect(m_lscQuotaDayMb->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                const quint32 mbytes = quint32(value);

                if (conf()->quotaDayMb() == mbytes)
                    return;

                conf()->setQuotaDayMb(mbytes);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupQuotaMonthMb()
{
    m_lscQuotaMonthMb = createSpinCombo(0, 1024 * 1024, " MiB");
    m_lscQuotaMonthMb->setValues(quotaValues);

    connect(m_lscQuotaMonthMb->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), this,
            [&](int value) {
                const quint32 mbytes = quint32(value);

                if (conf()->quotaMonthMb() == mbytes)
                    return;

                conf()->setQuotaMonthMb(mbytes);

                ctrl()->setConfFlagsEdited(true);
            });
}

void StatisticsPage::setupAppListView()
{
    m_appListView = new ListView();
    m_appListView->setFlow(QListView::TopToBottom);
    m_appListView->setViewMode(QListView::ListMode);
    m_appListView->setIconSize(QSize(24, 24));
    m_appListView->setUniformItemSizes(true);
    m_appListView->setAlternatingRowColors(true);

    m_appListView->setModel(appStatModel());
}

void StatisticsPage::setupTabBar()
{
    m_tabBar = new QTabBar();
    m_tabBar->setShape(QTabBar::TriangularNorth);

    for (int n = 4; --n >= 0;) {
        m_tabBar->addTab(QString());
    }
}

void StatisticsPage::setupTableTraf()
{
    m_tableTraf = new QTableView();
    m_tableTraf->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableTraf->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_tableTraf->setModel(trafListModel());

    const auto resetTableTraf = [&] {
        trafListModel()->setType(static_cast<TrafListModel::TrafType>(m_tabBar->currentIndex()));
        trafListModel()->setAppId(appStatModel()->appIdByRow(appListCurrentIndex()));
        trafListModel()->resetTraf();
    };

    resetTableTraf();

    connect(m_tabBar, &QTabBar::currentChanged, this, resetTableTraf);
    connect(m_appListView, &ListView::currentIndexChanged, this, resetTableTraf);

    const auto refreshTableTraf = [&] {
        trafListModel()->setUnit(static_cast<TrafListModel::TrafUnit>(conf()->trafUnit()));
        trafListModel()->refresh();
    };

    refreshTableTraf();

    connect(conf(), &FirewallConf::trafUnitChanged, this, refreshTableTraf);
}

void StatisticsPage::setupTableTrafHeader()
{
    auto header = m_tableTraf->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::Stretch);

    const auto refreshTableTrafHeader = [&] {
        auto hh = m_tableTraf->horizontalHeader();
        hh->resizeSection(0, qBound(150, qRound(hh->width() * 0.3), 180));
    };

    refreshTableTrafHeader();

    connect(header, &QHeaderView::geometriesChanged, this, refreshTableTrafHeader);
}

void StatisticsPage::setupAppInfoRow()
{
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    m_btAppCopyPath = ControlUtil::createLinkButton(":/icons/copy.png");
    m_btAppOpenFolder = ControlUtil::createLinkButton(":/icons/folder-open.png");

    m_lineAppPath = ControlUtil::createLineLabel();

    m_labelAppProductName = ControlUtil::createLabel();
    m_labelAppProductName->setFont(ControlUtil::fontDemiBold());

    m_labelAppCompanyName = ControlUtil::createLabel();

    connect(m_btAppCopyPath, &QAbstractButton::clicked, this,
            [&] { GuiUtil::setClipboardData(appListCurrentPath()); });
    connect(m_btAppOpenFolder, &QAbstractButton::clicked, this,
            [&] { OsUtil::openFolder(appListCurrentPath()); });

    layout->addWidget(m_btAppCopyPath);
    layout->addWidget(m_btAppOpenFolder);
    layout->addWidget(m_lineAppPath, 1);
    layout->addWidget(m_labelAppProductName);
    layout->addWidget(m_labelAppCompanyName);

    m_appInfoRow = new QWidget();
    m_appInfoRow->setLayout(layout);
}

void StatisticsPage::setupAppInfoVersion()
{
    const auto refreshAppInfoVersion = [&] {
        const auto appPath = appListCurrentPath();
        const auto appInfo = appInfoCache()->appInfo(appPath);

        m_lineAppPath->setText(appPath);
        m_lineAppPath->setToolTip(appPath);

        m_labelAppProductName->setVisible(!appInfo.productName.isEmpty());
        m_labelAppProductName->setText(appInfo.productName + " v" + appInfo.productVersion);

        m_labelAppCompanyName->setVisible(!appInfo.companyName.isEmpty());
        m_labelAppCompanyName->setText(appInfo.companyName);
    };

    refreshAppInfoVersion();

    connect(m_appListView, &ListView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void StatisticsPage::setupAppListViewChanged()
{
    const auto refreshAppListViewChanged = [&] {
        const bool appSelected = (appListCurrentIndex() > 0);
        m_actRemoveApp->setEnabled(appSelected);
        m_appInfoRow->setVisible(appSelected);
    };

    refreshAppListViewChanged();

    connect(m_appListView, &ListView::currentIndexChanged, this, refreshAppListViewChanged);
}

void StatisticsPage::updatePage()
{
    m_pageUpdating = true;

    m_cbLogStat->setChecked(conf()->logStat());

    m_ctpActivePeriod->checkBox()->setChecked(conf()->activePeriodEnabled());
    m_ctpActivePeriod->timeEdit1()->setTime(CheckTimePeriod::toTime(conf()->activePeriodFrom()));
    m_ctpActivePeriod->timeEdit2()->setTime(CheckTimePeriod::toTime(conf()->activePeriodTo()));

    m_lscMonthStart->spinBox()->setValue(conf()->monthStart());
    m_lscTrafHourKeepDays->spinBox()->setValue(conf()->trafHourKeepDays());
    m_lscTrafDayKeepDays->spinBox()->setValue(conf()->trafDayKeepDays());
    m_lscTrafMonthKeepMonths->spinBox()->setValue(conf()->trafMonthKeepMonths());

    m_cbLogAllowedIp->setChecked(conf()->logAllowedIp());
    m_lscAllowedIpKeepCount->spinBox()->setValue(conf()->allowedIpKeepCount());
    m_lscBlockedIpKeepCount->spinBox()->setValue(conf()->blockedIpKeepCount());

    m_lscQuotaDayMb->spinBox()->setValue(int(conf()->quotaDayMb()));
    m_lscQuotaMonthMb->spinBox()->setValue(int(conf()->quotaMonthMb()));

    m_cbGraphAlwaysOnTop->setChecked(settings()->graphWindowAlwaysOnTop());
    m_cbGraphFrameless->setChecked(settings()->graphWindowFrameless());
    m_cbGraphClickThrough->setChecked(settings()->graphWindowClickThrough());
    m_cbGraphHideOnHover->setChecked(settings()->graphWindowHideOnHover());

    m_graphOpacity->spinBox()->setValue(settings()->graphWindowOpacity());
    m_graphHoverOpacity->spinBox()->setValue(settings()->graphWindowHoverOpacity());
    m_graphMaxSeconds->spinBox()->setValue(settings()->graphWindowMaxSeconds());

    m_graphColor->setColor(settings()->graphWindowColor());
    m_graphColorIn->setColor(settings()->graphWindowColorIn());
    m_graphColorOut->setColor(settings()->graphWindowColorOut());
    m_graphAxisColor->setColor(settings()->graphWindowAxisColor());
    m_graphTickLabelColor->setColor(settings()->graphWindowTickLabelColor());
    m_graphLabelColor->setColor(settings()->graphWindowLabelColor());
    m_graphGridColor->setColor(settings()->graphWindowGridColor());

    updateTrafUnit();

    m_pageUpdating = false;
}

void StatisticsPage::updateTrafUnit()
{
    m_comboTrafUnit->setCurrentIndex(conf()->trafUnit());
}

int StatisticsPage::appListCurrentIndex() const
{
    return m_appListView->currentIndex().row();
}

QString StatisticsPage::appListCurrentPath() const
{
    return appStatModel()->appPathByRow(appListCurrentIndex());
}

LabelSpinCombo *StatisticsPage::createSpinCombo(int min, int max, const QString &suffix)
{
    auto c = new LabelSpinCombo();
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);
    return c;
}

LabelSpin *StatisticsPage::createSpin(int min, int max, const QString &suffix)
{
    auto c = new LabelSpin();
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);
    return c;
}

QString StatisticsPage::formatQuota(int mbytes)
{
    return NetUtil::formatDataSize1(qint64(mbytes) * 1024 * 1024);
}
