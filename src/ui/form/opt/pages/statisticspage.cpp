#include "statisticspage.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "../../../conf/firewallconf.h"
#include "../../../fortmanager.h"
#include "../../../fortsettings.h"
#include "../../../log/logmanager.h"
#include "../../../log/model/appstatmodel.h"
#include "../../../log/model/traflistmodel.h"
#include "../../../util/net/netutil.h"
#include "../../controls/checktimeperiod.h"
#include "../../controls/controlutil.h"
#include "../../controls/labelcolor.h"
#include "../../controls/labelspin.h"
#include "../../controls/labelspincombo.h"
#include "../optionscontroller.h"
#include "log/applistview.h"

namespace {

const ValuesList trafKeepDayValues = {
    60, -1, 90, 180, 365, 365 * 3
};
const ValuesList trafKeepMonthValues = {
    2, -1, 3, 6, 12, 36
};
const ValuesList quotaValues = {
    10, 0, 100, 500, 1024, 8 * 1024, 10 * 1024, 30 * 1024,
    50 * 1024, 100 * 1024
};

}

StatisticsPage::StatisticsPage(OptionsController *ctrl,
                               QWidget *parent) :
    BasePage(ctrl, parent)
{
    setupTrafListModel();

    setupUi();
    updatePage();
}

AppStatModel *StatisticsPage::appStatModel() const
{
    return fortManager()->logManager()->appStatModel();
}

void StatisticsPage::onEditResetted()
{
    setGraphEdited(false);
}

void StatisticsPage::onSaved()
{
    if (!graphEdited()) return;

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

void StatisticsPage::setGraphEdited(bool v)
{
    if (m_graphEdited != v) {
        m_graphEdited = v;

        if (graphEdited()) {
            ctrl()->setOthersEdited(true);
        }
    }
}

void StatisticsPage::onRetranslateUi()
{
    m_btRefresh->setText(tr("Refresh"));
    m_btClear->setText(tr("Clear"));

    m_actRemoveApp->setText(tr("Remove Application"));
    m_actResetTotal->setText(tr("Reset Total"));
    m_actClearAll->setText(tr("Clear All"));

    m_btTrafOptions->setText(tr("Options"));
    m_ctpActivePeriod->checkBox()->setText(tr("Active time period:"));
    m_lscMonthStart->label()->setText(tr("Month starts on:"));
    m_lscTrafHourKeepDays->label()->setText(tr("Keep data for 'Hourly':"));
    m_lscTrafHourKeepDays->spinBox()->setSuffix(tr(" days"));
    m_lscTrafDayKeepDays->label()->setText(tr("Keep data for 'Daily':"));
    m_lscTrafDayKeepDays->spinBox()->setSuffix(tr(" days"));
    m_lscTrafMonthKeepMonths->label()->setText(tr("Keep data for 'Monthly':"));
    m_lscTrafMonthKeepMonths->spinBox()->setSuffix(tr(" months"));
    m_lscQuotaDayMb->label()->setText(tr("Day's Quota:"));
    m_lscQuotaMonthMb->label()->setText(tr("Month's Quota:"));

    retranslateTrafKeepDayNames();
    retranslateTrafKeepMonthNames();
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

    m_cbLogStat->setText(tr("Collect Traffic Statistics"));
}

void StatisticsPage::retranslateTrafKeepDayNames()
{
    const QStringList list = {
        tr("Custom"), tr("Forever"), tr("3 months"),
        tr("6 months"), tr("1 year"), tr("3 years")
    };

    m_lscTrafHourKeepDays->setNames(list);
    m_lscTrafDayKeepDays->setNames(list);
}

void StatisticsPage::retranslateTrafKeepMonthNames()
{
    const QStringList list = {
        tr("Custom"), tr("Forever"), tr("3 months"),
        tr("6 months"), tr("1 year"), tr("3 years")
    };

    m_lscTrafMonthKeepMonths->setNames(list);
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
    const QStringList list = {
        tr("Adaptive"), tr("Bytes"), "KiB", "MiB", "GiB", "TiB"
    };

    m_comboTrafUnit->clear();
    m_comboTrafUnit->addItems(list);

    updateTrafUnit();
}

void StatisticsPage::setupTrafListModel()
{
    m_trafListModel = appStatModel()->trafListModel();

//    m_trafListModel->setType(static_cast<TrafListModel::TrafType>(tabBar.currentIndex));
//    m_trafListModel->setAppId(appStatModel()->appIdByRow(appListView.currentIndex));
//    m_trafListModel->reset();
}

void StatisticsPage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Content
    setupAppListView();

    layout->addWidget(m_appListView, 1);

    this->setLayout(layout);
}

QLayout *StatisticsPage::setupHeader()
{
    auto layout = new QHBoxLayout();

    m_btRefresh = ControlUtil::createButton(":/images/arrow_refresh.png", [&] {
        trafListModel()->refresh();
    });

    setupClearMenu();
    setupTrafOptionsMenu();
    setupGraphOptionsMenu();
    setupTrafUnits();
    setupLogStat();

    layout->addWidget(m_btRefresh);
    layout->addWidget(m_btClear);
    layout->addWidget(m_btTrafOptions);
    layout->addWidget(m_btGraphOptions);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addWidget(m_traphUnits);
    layout->addWidget(m_comboTrafUnit);
    layout->addStretch();
    layout->addWidget(m_cbLogStat);

    return layout;
}

void StatisticsPage::setupClearMenu()
{
    auto menu = new QMenu(this);

    m_actRemoveApp = menu->addAction(QIcon(":/images/application_delete.png"), QString());
    m_actRemoveApp->setShortcut(Qt::Key_Delete);

    m_actResetTotal = menu->addAction(QString());
    m_actClearAll = menu->addAction(QString());

    connect(m_actRemoveApp, &QAction::triggered, [&] {
        if (!fortManager()->showQuestionBox(tr("Are you sure to remove statistics for selected application?")))
            return;

        //appStatModel()->remove(appListView.currentIndex);
    });
    connect(m_actResetTotal, &QAction::triggered, [&] {
        if (!fortManager()->showQuestionBox(tr("Are you sure to reset total statistics?")))
            return;

        trafListModel()->resetAppTotals();
    });
    connect(m_actClearAll, &QAction::triggered, [&] {
        if (!fortManager()->showQuestionBox(tr("Are you sure to clear all statistics?")))
            return;

        //appListView.currentIndex = 0;
        appStatModel()->clear();
    });

    m_btClear = new QPushButton(QIcon(":/images/bin_empty.png"), QString());
    m_btClear->setMenu(menu);
}

void StatisticsPage::setupTrafOptionsMenu()
{
    setupActivePeriod();
    setupMonthStart();
    setupTrafHourKeepDays();
    setupTrafDayKeepDays();
    setupTrafMonthKeepMonths();
    setupQuotaDayMb();
    setupQuotaMonthMb();

    // Menu
    const QList<QWidget *> menuWidgets = {
        m_ctpActivePeriod, m_lscMonthStart,
        ControlUtil::createSeparator(),
        m_lscTrafHourKeepDays, m_lscTrafDayKeepDays, m_lscTrafMonthKeepMonths,
        ControlUtil::createSeparator(),
        m_lscQuotaDayMb, m_lscQuotaMonthMb
    };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btTrafOptions = new QPushButton(QIcon(":/images/database_save.png"), QString());
    m_btTrafOptions->setMenu(menu);
}

void StatisticsPage::setupActivePeriod()
{
    m_ctpActivePeriod = new CheckTimePeriod();

    connect(m_ctpActivePeriod->checkBox(), &QCheckBox::toggled, [&](bool checked) {
        if (conf()->activePeriodEnabled() == checked)
            return;

        conf()->setActivePeriodEnabled(checked);

        ctrl()->setConfFlagsEdited(true);
    });
    connect(m_ctpActivePeriod->timeEdit1(), &QTimeEdit::userTimeChanged, [&](const QTime &time) {
        const auto timeStr = CheckTimePeriod::fromTime(time);

        if (conf()->activePeriodFrom() == timeStr)
            return;

        conf()->setActivePeriodFrom(timeStr);

        ctrl()->setConfFlagsEdited(true);
    });
    connect(m_ctpActivePeriod->timeEdit2(), &QTimeEdit::userTimeChanged, [&](const QTime &time) {
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

    connect(m_lscMonthStart->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
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

    connect(m_lscTrafHourKeepDays->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
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

    connect(m_lscTrafDayKeepDays->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
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

    connect(m_lscTrafMonthKeepMonths->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
        if (conf()->trafMonthKeepMonths() == value)
            return;

        conf()->setTrafMonthKeepMonths(value);

        ctrl()->setConfFlagsEdited(true);
    });
}

void StatisticsPage::setupQuotaDayMb()
{
    m_lscQuotaDayMb = createSpinCombo(0, 1024 * 1024, " MiB");
    m_lscQuotaDayMb->setValues(quotaValues);

    connect(m_lscQuotaDayMb->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
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

    connect(m_lscQuotaMonthMb->spinBox(), QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
        const quint32 mbytes = quint32(value);

        if (conf()->quotaMonthMb() == mbytes)
            return;

        conf()->setQuotaMonthMb(mbytes);

        ctrl()->setConfFlagsEdited(true);
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
    auto colLayout1 = ControlUtil::createLayoutByWidgets({
                                                             m_cbGraphAlwaysOnTop, m_cbGraphFrameless,
                                                             m_cbGraphClickThrough, m_cbGraphHideOnHover,
                                                             ControlUtil::createSeparator(),
                                                             m_graphOpacity, m_graphHoverOpacity,
                                                             m_graphMaxSeconds, nullptr
                                                         });
    auto colLayout2 = ControlUtil::createLayoutByWidgets({
                                                             m_graphColor, m_graphColorIn, m_graphColorOut,
                                                             m_graphAxisColor, m_graphTickLabelColor,
                                                             m_graphLabelColor, m_graphGridColor
                                                         });
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addWidget(ControlUtil::createSeparator(Qt::Vertical));
    layout->addLayout(colLayout2);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btGraphOptions = new QPushButton(QIcon(":/images/chart_bar.png"), QString());
    m_btGraphOptions->setMenu(menu);
}

void StatisticsPage::setupTrafUnits()
{
    m_traphUnits = new QLabel();

    m_comboTrafUnit = ControlUtil::createComboBox(QStringList(), [&](int index) {
        if (conf()->trafUnit() == index)
            return;

        conf()->setTrafUnit(index);

        fortManager()->applyConfImmediateFlags();

        trafListModel()->refresh();
    });
}

void StatisticsPage::setupLogStat()
{
    m_cbLogStat = ControlUtil::createCheckBox(false, [&](bool checked) {
        if (conf()->logStat() == checked)
            return;

        conf()->setLogStat(checked);

        fortManager()->applyConfImmediateFlags();
    });

    m_cbLogStat->setFont(ControlUtil::createFont(QFont::DemiBold));
}

void StatisticsPage::setupAppListView()
{
    m_appListView = new AppListView();

    m_appListView->setModel(appStatModel());
}

void StatisticsPage::updatePage()
{
    m_pageUpdating = true;

    m_ctpActivePeriod->checkBox()->setChecked(conf()->activePeriodEnabled());
    m_ctpActivePeriod->timeEdit1()->setTime(CheckTimePeriod::toTime(
                                                conf()->activePeriodFrom()));
    m_ctpActivePeriod->timeEdit2()->setTime(CheckTimePeriod::toTime(
                                                conf()->activePeriodTo()));

    m_lscMonthStart->spinBox()->setValue(conf()->monthStart());
    m_lscTrafHourKeepDays->spinBox()->setValue(conf()->trafHourKeepDays());
    m_lscTrafDayKeepDays->spinBox()->setValue(conf()->trafDayKeepDays());
    m_lscTrafMonthKeepMonths->spinBox()->setValue(conf()->trafMonthKeepMonths());
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

    m_cbLogStat->setChecked(conf()->logStat());

    m_pageUpdating = false;
}

void StatisticsPage::updateTrafUnit()
{
    m_comboTrafUnit->setCurrentIndex(conf()->trafUnit());
}

LabelSpinCombo *StatisticsPage::createSpinCombo(int min, int max,
                                                const QString &suffix)
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
