#ifndef STATISTICSPAGE_H
#define STATISTICSPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTableView)

class AppInfoCache;
class AppInfoRow;
class AppStatModel;
class CheckTimePeriod;
class LabelColor;
class LabelSpin;
class LabelSpinCombo;
class ListView;
class TrafListModel;

class StatisticsPage : public BasePage
{
    Q_OBJECT

public:
    explicit StatisticsPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    AppStatModel *appStatModel() const;
    AppInfoCache *appInfoCache() const;
    TrafListModel *trafListModel() const { return m_trafListModel; }

    void setIniEdited();

signals:
    void trafUnitChanged();

protected slots:
    void onSaveWindowState(IniOptions *ini) override;
    void onRestoreWindowState(IniOptions *ini) override;

    void onRetranslateUi() override;

private:
    void retranslateTrafKeepDayNames();
    void retranslateTrafKeepMonthNames();
    void retranslateQuotaNames();
    void retranslateIpKeepCountNames();
    void retranslateTrafUnitNames();
    void retranslateTabBar();

    void setupTrafListModel();

    void setupUi();
    QLayout *setupHeader();
    void setupClearMenu();
    void setupTrafUnits();
    void setupGraphOptionsMenu();
    void setupTrafOptionsMenu();
    void setupLogStat();
    void setupLogStatNoFilter();
    void setupActivePeriod();
    void setupMonthStart();
    void setupTrafHourKeepDays();
    void setupTrafDayKeepDays();
    void setupTrafMonthKeepMonths();
    void setupQuotaDayMb();
    void setupQuotaMonthMb();
    void setupAllowedIpKeepCount();
    void setupBlockedIpKeepCount();
    void setupAppListView();
    void setupTabBar();
    void setupTableTraf();
    void setupTableTrafHeader();
    void setupAppInfoRow();
    void setupAppListViewChanged();
    void updatePage();
    void updateTrafUnit();

    int appListCurrentIndex() const;
    QString appListCurrentPath() const;

    static LabelSpinCombo *createSpinCombo(int min, int max, const QString &suffix = QString());
    static LabelSpin *createSpin(int min, int max, const QString &suffix = QString());

    static QString formatQuota(int mbytes);

private:
    bool m_isPageUpdating : 1;

    TrafListModel *m_trafListModel = nullptr;

    QPushButton *m_btRefresh = nullptr;
    QPushButton *m_btClear = nullptr;
    QAction *m_actRemoveApp = nullptr;
    QAction *m_actResetTotal = nullptr;
    QAction *m_actClearAll = nullptr;
    QLabel *m_traphUnits = nullptr;
    QComboBox *m_comboTrafUnit = nullptr;
    QPushButton *m_btGraphOptions = nullptr;
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
    QPushButton *m_btTrafOptions = nullptr;
    QCheckBox *m_cbLogStat = nullptr;
    QCheckBox *m_cbLogStatNoFilter = nullptr;
    CheckTimePeriod *m_ctpActivePeriod = nullptr;
    LabelSpinCombo *m_lscMonthStart = nullptr;
    LabelSpinCombo *m_lscTrafHourKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafDayKeepDays = nullptr;
    LabelSpinCombo *m_lscTrafMonthKeepMonths = nullptr;
    LabelSpinCombo *m_lscQuotaDayMb = nullptr;
    LabelSpinCombo *m_lscQuotaMonthMb = nullptr;
    LabelSpinCombo *m_lscAllowedIpKeepCount = nullptr;
    LabelSpinCombo *m_lscBlockedIpKeepCount = nullptr;
    QSplitter *m_splitter = nullptr;
    ListView *m_appListView = nullptr;
    QTabBar *m_tabBar = nullptr;
    QTableView *m_tableTraf = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // STATISTICSPAGE_H
