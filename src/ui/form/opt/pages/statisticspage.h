#ifndef STATISTICSPAGE_H
#define STATISTICSPAGE_H

#include "optbasepage.h"

class CheckTimePeriod;
class LabelSpinCombo;

class StatisticsPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit StatisticsPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

public slots:
    void onResetToDefault() override;

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
    void setupBlockedConnBox();
    void setupLogBlockedIp();
    void setupAllowedConnBox();
    void setupLogAllowedIp();
    QLayout *setupColumn2();

private:
    QGroupBox *m_gbTraffic = nullptr;
    QGroupBox *m_gbBlockedConn = nullptr;
    QGroupBox *m_gbAllowedConn = nullptr;
    QCheckBox *m_cbLogStat = nullptr;
    QCheckBox *m_cbLogStatNoFilter = nullptr;
    CheckTimePeriod *m_ctpActivePeriod = nullptr;
    LabelSpinCombo *m_lscMonthStart = nullptr;
    LabelSpinCombo *m_lscTrafHourKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafDayKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafMonthKeepMonths = nullptr;
    LabelSpinCombo *m_lscQuotaDayMb = nullptr;
    LabelSpinCombo *m_lscQuotaMonthMb = nullptr;
    QCheckBox *m_cbQuotaBlockInternet = nullptr;
    QCheckBox *m_cbLogBlockedIp = nullptr;
    QCheckBox *m_cbLogAlertedBlockedIp = nullptr;
    LabelSpinCombo *m_lscBlockedIpKeepCount = nullptr;
    QCheckBox *m_cbLogAllowedIp = nullptr;
    LabelSpinCombo *m_lscAllowedIpKeepCount = nullptr;
};

#endif // STATISTICSPAGE_H
