#ifndef TRAFFICPAGE_H
#define TRAFFICPAGE_H

#include "statbasepage.h"

QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTableView)

class AppInfoCache;
class AppInfoRow;
class AppStatModel;
class ListView;
class TrafListModel;

class TrafficPage : public StatBasePage
{
    Q_OBJECT

public:
    explicit TrafficPage(StatisticsController *ctrl = nullptr, QWidget *parent = nullptr);

    AppStatModel *appStatModel() const { return m_appStatModel; }
    TrafListModel *trafListModel() const { return m_trafListModel; }
    AppInfoCache *appInfoCache() const;

protected slots:
    void onSaveWindowState(IniUser *ini) override;
    void onRestoreWindowState(IniUser *ini) override;

    void onRetranslateUi() override;

private:
    void retranslateTrafUnitNames();
    void retranslateTabBar();

    void setupUi();
    QLayout *setupHeader();
    void setupClearMenu();
    void setupRefresh();
    void setupTrafUnits();
    void setupAppListView();
    void setupTabBar();
    void setupTableTraf();
    void setupTableTrafHeader();
    void setupAppInfoRow();
    void setupAppListViewChanged();

    void updateTrafUnit();
    void updateTableTrafUnit();

    int appListCurrentIndex() const;
    QString appListCurrentPath() const;

private:
    AppStatModel *m_appStatModel = nullptr;
    TrafListModel *m_trafListModel = nullptr;

    QPushButton *m_btClear = nullptr;
    QAction *m_actRemoveApp = nullptr;
    QAction *m_actResetTotal = nullptr;
    QAction *m_actClearAll = nullptr;
    QToolButton *m_btRefresh = nullptr;
    QLabel *m_traphUnits = nullptr;
    QComboBox *m_comboTrafUnit = nullptr;
    QSplitter *m_splitter = nullptr;
    ListView *m_appListView = nullptr;
    QTabBar *m_tabBar = nullptr;
    QTableView *m_tableTraf = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // TRAFFICPAGE_H
