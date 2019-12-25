#include "statisticspage.h"

#include <QAction>
#include <QCheckBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

#include "../../../conf/firewallconf.h"
#include "../../../fortmanager.h"
#include "../../../log/logmanager.h"
#include "../../../log/model/appstatmodel.h"
#include "../../../log/model/traflistmodel.h"
#include "../../../util/net/netutil.h"
#include "../../controls/checktimeperiod.h"
#include "../../controls/controlutil.h"
#include "../../controls/labelspincombo.h"
#include "../optionscontroller.h"

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
    setupUi();
    refreshPage();

    setupModels();
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

    //graphButton.save();

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
    retranslateTrafUnitNames();
}

void StatisticsPage::retranslateTrafKeepDayNames() const
{
    const QStringList list = {
        tr("Custom"), tr("Forever"), tr("3 months"),
        tr("6 months"), tr("1 year"), tr("3 years")
    };

    m_lscTrafHourKeepDays->setNames(list);
    m_lscTrafDayKeepDays->setNames(list);
}

void StatisticsPage::retranslateTrafKeepMonthNames() const
{
    const QStringList list = {
        tr("Custom"), tr("Forever"), tr("3 months"),
        tr("6 months"), tr("1 year"), tr("3 years")
    };

    m_lscTrafMonthKeepMonths->setNames(list);
}

void StatisticsPage::retranslateQuotaNames() const
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

void StatisticsPage::retranslateTrafUnitNames() const
{
    //return {tr("Adaptive"), tr("Bytes"), tr("KiB"), tr("MiB"), tr("GiB"), tr("TiB")};
}

void StatisticsPage::setupUi()
{
    auto layout = new QVBoxLayout();

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    layout->addStretch();

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

    layout->addWidget(m_btRefresh);
    layout->addWidget(m_btClear);
    layout->addWidget(m_btTrafOptions);
    layout->addStretch();

    return layout;
}

void StatisticsPage::setupClearMenu()
{
    auto menu = new QMenu(this);

    m_actRemoveApp = menu->addAction(QIcon(":/images/application_delete.png"), QString());
    m_actResetTotal = menu->addAction(QString());
    m_actClearAll = menu->addAction(QString());

    connect(m_actRemoveApp, &QAction::triggered, [&] {
        //appStatModel()->remove(appListView.currentIndex);
    });
    connect(m_actResetTotal, &QAction::triggered, [&] {
        trafListModel()->resetAppTotals();
    });
    connect(m_actClearAll, &QAction::triggered, [&] {
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
    auto menu = ControlUtil::createMenuByWidgets(menuWidgets, this);

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
void StatisticsPage::refreshPage()
{
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
}

void StatisticsPage::setupModels()
{
    m_trafListModel = appStatModel()->trafListModel();

//    m_trafListModel->setType(static_cast<TrafListModel::TrafType>(tabBar.currentIndex));
//    m_trafListModel->setAppId(appStatModel()->appIdByRow(appListView.currentIndex));
//    m_trafListModel->reset();
}

LabelSpinCombo *StatisticsPage::createSpinCombo(int min, int max,
                                                const QString &suffix)
{
    auto c = new LabelSpinCombo();
    c->spinBox()->setRange(min, max);
    c->spinBox()->setSuffix(suffix);
    return c;
}

QString StatisticsPage::formatQuota(int mbytes)
{
    return NetUtil::formatDataSize1(qint64(mbytes) * 1024 * 1024);
}
