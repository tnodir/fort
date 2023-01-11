#ifndef STATISTICSPAGE_H
#define STATISTICSPAGE_H

#include "optbasepage.h"

class CheckTimePeriod;
class LabelColor;
class LabelSpin;
class LabelSpinCombo;

class StatisticsPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit StatisticsPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void retranslateTrafKeepDayNames();
    void retranslateTrafKeepMonthNames();
    void retranslateQuotaNames();
    void retranslateIpKeepCountNames();

    void setupUi();
    QLayout *setupColumn1();
    void setupTrafficBox();
    void setupLogStat();
    void setupLogStatNoFilter();
    void setupActivePeriod();
    void setupMonthStart();
    void setupTrafKeep();
    void setupQuota();
    void setupConnBox();
    void setupLogAllowedIp();
    void setupLogBlockedIp();
    void setupProgBox();
    void setupLogBlocked();
    QLayout *setupColumn2();
    void setupGraphBox();
    void setupGraphCheckboxes();
    void setupGraphOptions();
    void setupGraphColors();

private:
    QGroupBox *m_gbTraffic = nullptr;
    QGroupBox *m_gbConn = nullptr;
    QGroupBox *m_gbProg = nullptr;
    QGroupBox *m_gbGraph = nullptr;
    QCheckBox *m_cbLogStat = nullptr;
    QCheckBox *m_cbLogStatNoFilter = nullptr;
    CheckTimePeriod *m_ctpActivePeriod = nullptr;
    LabelSpinCombo *m_lscMonthStart = nullptr;
    LabelSpinCombo *m_lscTrafHourKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafDayKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafMonthKeepMonths = nullptr;
    LabelSpinCombo *m_lscQuotaDayMb = nullptr;
    LabelSpinCombo *m_lscQuotaMonthMb = nullptr;
    QCheckBox *m_cbLogAllowedIp = nullptr;
    LabelSpinCombo *m_lscAllowedIpKeepCount = nullptr;
    QCheckBox *m_cbLogBlockedIp = nullptr;
    LabelSpinCombo *m_lscBlockedIpKeepCount = nullptr;
    QCheckBox *m_cbLogBlocked = nullptr;
    QCheckBox *m_cbGraphAlwaysOnTop = nullptr;
    QCheckBox *m_cbGraphFrameless = nullptr;
    QCheckBox *m_cbGraphClickThrough = nullptr;
    QCheckBox *m_cbGraphHideOnHover = nullptr;
    LabelSpin *m_graphOpacity = nullptr;
    LabelSpin *m_graphHoverOpacity = nullptr;
    LabelSpin *m_graphMaxSeconds = nullptr;
    LabelColor *m_graphColor = nullptr;
    LabelColor *m_graphColorIn = nullptr;
    LabelColor *m_graphColorOut = nullptr;
    LabelColor *m_graphAxisColor = nullptr;
    LabelColor *m_graphTickLabelColor = nullptr;
    LabelColor *m_graphLabelColor = nullptr;
    LabelColor *m_graphGridColor = nullptr;
};

#endif // STATISTICSPAGE_H
