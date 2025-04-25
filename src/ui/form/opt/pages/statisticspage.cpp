#include "statisticspage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <conf/inioptions.h>
#include <form/controls/checktimeperiod.h>
#include <form/controls/controlutil.h>
#include <form/controls/labelcolor.h>
#include <form/controls/labelspin.h>
#include <form/controls/labelspincombo.h>
#include <form/opt/optionscontroller.h>
#include <util/formatutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

namespace {

const std::array dayValues = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
const std::array trafKeepDayValues = { 60, -1, 90, 180, 365, 365 * 3, 365 * 5, 365 * 10 };
const std::array trafKeepMonthValues = { 2, -1, 3, 6, 12, 12 * 3, 12 * 5, 12 * 10 };
const std::array logIpKeepCountValues = { 3000, 1000, 5000, 10000, 50000, 100000, 500000, 1000000,
    5000000, 10000000 };
const std::array quotaValues = { 10, 0, 100, 500, 1024, 8 * 1024, 10 * 1024, 30 * 1024, 50 * 1024,
    100 * 1024 };

QString formatQuota(int mbytes)
{
    return FormatUtil::formatDataSize(qint64(mbytes) * 1024 * 1024, /*precision=*/0);
}

}

StatisticsPage::StatisticsPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void StatisticsPage::onResetToDefault()
{
    m_cbLogStat->setChecked(true);
    m_cbLogStatNoFilter->setChecked(true);

    m_ctpActivePeriod->checkBox()->setChecked(false);

    m_lscMonthStart->spinBox()->setValue(DEFAULT_MONTH_START);
    m_lscTrafHourKeepDays->spinBox()->setValue(DEFAULT_TRAF_HOUR_KEEP_DAYS);
    m_lscTrafDayKeepDays->spinBox()->setValue(DEFAULT_TRAF_DAY_KEEP_DAYS);
    m_lscTrafMonthKeepMonths->spinBox()->setValue(DEFAULT_TRAF_MONTH_KEEP_MONTHS);

    m_lscQuotaDayMb->spinBox()->setValue(0);
    m_lscQuotaMonthMb->spinBox()->setValue(0);
    m_cbQuotaBlockInternet->setChecked(false);

    m_cbLogAllowedConn->setChecked(false);
    m_cbLogBlockedConn->setChecked(true);
    m_cbLogAlertedConn->setChecked(false);
    m_cbClearConnOnExit->setChecked(true);
    m_lscConnKeepCount->spinBox()->setValue(DEFAULT_LOG_CONN_KEEP_COUNT);
}

void StatisticsPage::onRetranslateUi()
{
    m_gbTraffic->setTitle(tr("Traffic"));
    m_gbConn->setTitle(tr("Connections"));

    m_cbLogStat->setText(tr("Collect Traffic Statistics"));
    m_cbLogStatNoFilter->setText(tr("Collect Traffic, when Filter Disabled"));
    m_ctpActivePeriod->checkBox()->setText(tr("Active time period:"));
    m_lscMonthStart->label()->setText(tr("Month starts on:"));

    m_lscTrafHourKeepDays->label()->setText(tr("Keep data for 'Hourly':"));
    m_lscTrafHourKeepDays->spinBox()->setSuffix(tr(" day(s)"));
    m_lscTrafDayKeepDays->label()->setText(tr("Keep data for 'Daily':"));
    m_lscTrafDayKeepDays->spinBox()->setSuffix(tr(" day(s)"));
    m_lscTrafMonthKeepMonths->label()->setText(tr("Keep data for 'Monthly':"));
    m_lscTrafMonthKeepMonths->spinBox()->setSuffix(tr(" month(s)"));

    m_lscQuotaDayMb->label()->setText(tr("Day's Quota:"));
    m_lscQuotaMonthMb->label()->setText(tr("Month's Quota:"));
    m_cbQuotaBlockInternet->setText(tr("Block Internet traffic when quota exceeds"));

    m_cbLogAllowedConn->setText(tr("Collect allowed connections"));
    m_cbLogBlockedConn->setText(tr("Collect blocked connections"));
    m_cbLogAlertedConn->setText(tr("Alerted only"));
    m_cbClearConnOnExit->setText(tr("Clear connections on exit (relax disk writes)"));
    m_lscConnKeepCount->label()->setText(tr("Keep count for connections:"));

    retranslateTrafKeepDayNames();
    retranslateTrafKeepMonthNames();
    retranslateQuotaNames();
    retranslateConnKeepCountNames();
}

void StatisticsPage::retranslateTrafKeepDayNames()
{
    const QStringList list = { tr("Custom"), tr("Forever"), tr("3 months"), tr("6 months"),
        tr("1 year"), tr("3 years"), tr("5 years"), tr("10 years") };

    m_lscTrafHourKeepDays->setNames(list);
    m_lscTrafDayKeepDays->setNames(list);
}

void StatisticsPage::retranslateTrafKeepMonthNames()
{
    const QStringList list = { tr("Custom"), tr("Forever"), tr("3 months"), tr("6 months"),
        tr("1 year"), tr("3 years"), tr("5 years"), tr("10 years") };

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

void StatisticsPage::retranslateConnKeepCountNames()
{
    const QStringList list = { tr("Custom"), "1K", "5K", "10K", "50K", "100K", "500K", "1M", "5M",
        "10M" };

    m_lscConnKeepCount->setNames(list);
}

void StatisticsPage::setupUi()
{
    // Column #1
    auto colLayout1 = setupColumn1();

    // Column #2
    auto colLayout2 = setupColumn2();

    // Main layout
    auto layout = new QHBoxLayout();
    layout->addLayout(colLayout1);
    layout->addStretch();
    layout->addLayout(colLayout2);
    layout->addStretch();

    this->setLayout(layout);
}

QLayout *StatisticsPage::setupColumn1()
{
    // Traffic Group Box
    setupTrafficBox();

    // Blocked Connections Group Box
    setupConnBox();

    auto layout =
            ControlUtil::createVLayoutByWidgets({ m_gbTraffic, m_gbConn, /*stretch*/ nullptr });
    layout->setSpacing(10);

    return layout;
}

QLayout *StatisticsPage::setupColumn2()
{
    auto layout = new QVBoxLayout();
    layout->setSpacing(10);

    layout->addStretch();

    return layout;
}

void StatisticsPage::setupTrafficBox()
{
    setupLogStat();
    setupLogStatNoFilter();
    setupActivePeriod();
    setupMonthStart();
    setupTrafKeep();
    setupQuota();

    // Layout
    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbLogStat, m_cbLogStatNoFilter, m_ctpActivePeriod, m_lscMonthStart,
                    ControlUtil::createSeparator(), m_lscTrafHourKeepDays, m_lscTrafDayKeepDays,
                    m_lscTrafMonthKeepMonths, ControlUtil::createSeparator(), m_lscQuotaDayMb,
                    m_lscQuotaMonthMb, m_cbQuotaBlockInternet });

    m_gbTraffic = new QGroupBox();
    m_gbTraffic->setLayout(layout);
}

void StatisticsPage::setupLogStat()
{
    m_cbLogStat = ControlUtil::createCheckBox(conf()->logStat(), [&](bool checked) {
        if (conf()->logStat() != checked) {
            conf()->setLogStat(checked);
            ctrl()->setFlagsEdited();
        }
    });

    m_cbLogStat->setFont(GuiUtil::fontBold());
}

void StatisticsPage::setupLogStatNoFilter()
{
    m_cbLogStatNoFilter = ControlUtil::createCheckBox(conf()->logStatNoFilter(), [&](bool checked) {
        if (conf()->logStatNoFilter() != checked) {
            conf()->setLogStatNoFilter(checked);
            ctrl()->setFlagsEdited();
        }
    });
}

void StatisticsPage::setupActivePeriod()
{
    m_ctpActivePeriod = new CheckTimePeriod();
    m_ctpActivePeriod->checkBox()->setChecked(conf()->activePeriodEnabled());
    m_ctpActivePeriod->timeEdit1()->setTime(CheckTimePeriod::toTime(conf()->activePeriodFrom()));
    m_ctpActivePeriod->timeEdit2()->setTime(CheckTimePeriod::toTime(conf()->activePeriodTo()));

    connect(m_ctpActivePeriod->checkBox(), &QCheckBox::toggled, this, [&](bool checked) {
        if (conf()->activePeriodEnabled() != checked) {
            conf()->setActivePeriodEnabled(checked);
            ctrl()->setFlagsEdited();
        }
    });
    connect(m_ctpActivePeriod->timeEdit1(), &QTimeEdit::userTimeChanged, this,
            [&](const QTime &time) {
                const auto timeStr = CheckTimePeriod::fromTime(time);

                if (conf()->activePeriodFrom() != timeStr) {
                    conf()->setActivePeriodFrom(timeStr);
                    ctrl()->setFlagsEdited();
                }
            });
    connect(m_ctpActivePeriod->timeEdit2(), &QTimeEdit::userTimeChanged, this,
            [&](const QTime &time) {
                const auto timeStr = CheckTimePeriod::fromTime(time);

                if (conf()->activePeriodTo() != timeStr) {
                    conf()->setActivePeriodTo(timeStr);
                    ctrl()->setFlagsEdited();
                }
            });
}

void StatisticsPage::setupMonthStart()
{
    const auto dayList = SpinCombo::makeValuesList(dayValues);
    m_lscMonthStart =
            ControlUtil::createSpinCombo(ini()->monthStart(), 1, 31, dayList, {}, [&](int value) {
                if (ini()->monthStart() != value) {
                    ini()->setMonthStart(value);
                    ctrl()->setIniEdited();
                }
            });
    m_lscMonthStart->setNamesByValues();
}

void StatisticsPage::setupTrafKeep()
{
    const auto trafKeepDayList = SpinCombo::makeValuesList(trafKeepDayValues);
    m_lscTrafHourKeepDays = ControlUtil::createSpinCombo(
            ini()->trafHourKeepDays(), -1, 9999, trafKeepDayList, {}, [&](int value) {
                if (ini()->trafHourKeepDays() != value) {
                    ini()->setTrafHourKeepDays(value);
                    ctrl()->setIniEdited();
                }
            });

    m_lscTrafDayKeepDays = ControlUtil::createSpinCombo(
            ini()->trafDayKeepDays(), -1, 9999, trafKeepDayList, {}, [&](int value) {
                if (ini()->trafDayKeepDays() != value) {
                    ini()->setTrafDayKeepDays(value);
                    ctrl()->setIniEdited();
                }
            });

    const auto trafKeepMonthList = SpinCombo::makeValuesList(trafKeepMonthValues);
    m_lscTrafMonthKeepMonths = ControlUtil::createSpinCombo(
            ini()->trafMonthKeepMonths(), -1, 9999, trafKeepMonthList, {}, [&](int value) {
                if (ini()->trafMonthKeepMonths() != value) {
                    ini()->setTrafMonthKeepMonths(value);
                    ctrl()->setIniEdited();
                }
            });
}

void StatisticsPage::setupQuota()
{
    const auto quotaList = SpinCombo::makeValuesList(quotaValues);
    m_lscQuotaDayMb = ControlUtil::createSpinCombo(
            ini()->quotaDayMb(), 0, 1024 * 1024, quotaList, " MiB", [&](int value) {
                if (ini()->quotaDayMb() != value) {
                    ini()->setQuotaDayMb(value);
                    ctrl()->setIniEdited();
                }
            });

    m_lscQuotaMonthMb = ControlUtil::createSpinCombo(
            ini()->quotaMonthMb(), 0, 1024 * 1024, quotaList, " MiB", [&](int value) {
                if (ini()->quotaMonthMb() != value) {
                    ini()->setQuotaMonthMb(value);
                    ctrl()->setIniEdited();
                }
            });

    m_cbQuotaBlockInternet =
            ControlUtil::createCheckBox(ini()->quotaBlockInetTraffic(), [&](bool checked) {
                if (ini()->quotaBlockInetTraffic() != checked) {
                    ini()->setQuotaBlockInternet(checked);
                    ctrl()->setIniEdited();
                }
            });
}

void StatisticsPage::setupConnBox()
{
    setupLogConn();

    // Layout
    auto layout = ControlUtil::createVLayoutByWidgets(
            { m_cbLogAllowedConn, m_cbLogBlockedConn, m_cbLogAlertedConn,
                    ControlUtil::createSeparator(), m_cbClearConnOnExit, m_lscConnKeepCount });

    m_gbConn = new QGroupBox();
    m_gbConn->setLayout(layout);
}

void StatisticsPage::setupLogConn()
{
    // Allowed Connection
    m_cbLogAllowedConn = ControlUtil::createCheckBox(conf()->logAllowedConn(), [&](bool checked) {
        if (conf()->logAllowedConn() != checked) {
            conf()->setLogAllowedConn(checked);
            ctrl()->setFlagsEdited();
        }
    });
    m_cbLogAllowedConn->setFont(GuiUtil::fontBold());

    // Blocked Connection
    m_cbLogBlockedConn = ControlUtil::createCheckBox(conf()->logBlockedConn(), [&](bool checked) {
        if (conf()->logBlockedConn() != checked) {
            conf()->setLogBlockedConn(checked);
            ctrl()->setFlagsEdited();
        }
    });
    m_cbLogBlockedConn->setFont(GuiUtil::fontBold());

    // Alerted Connection
    m_cbLogAlertedConn = ControlUtil::createCheckBox(conf()->logAlertedConn(), [&](bool checked) {
        if (conf()->logAlertedConn() != checked) {
            conf()->setLogAlertedConn(checked);
            ctrl()->setFlagsEdited();
        }
    });

    // Clear Connections on Exit
    m_cbClearConnOnExit = ControlUtil::createCheckBox(conf()->clearConnOnExit(), [&](bool checked) {
        if (conf()->clearConnOnExit() != checked) {
            conf()->setClearConnOnExit(checked);
            ctrl()->setFlagsEdited();
        }
    });

    // Connections Keep Count
    const auto logConnKeepCountList = SpinCombo::makeValuesList(logIpKeepCountValues);
    m_lscConnKeepCount = ControlUtil::createSpinCombo(
            ini()->connKeepCount(), 0, 999999999, logConnKeepCountList, {}, [&](int value) {
                if (ini()->connKeepCount() != value) {
                    ini()->setConnKeepCount(value);
                    ctrl()->setIniEdited();
                }
            });
}
