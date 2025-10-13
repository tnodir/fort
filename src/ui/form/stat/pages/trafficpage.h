#ifndef TRAFFICPAGE_H
#define TRAFFICPAGE_H

#include <QModelIndex>

#include "statbasepage.h"

QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTableView)

class AppInfoRow;
class AppStatModel;
class TableView;
class TrafListModel;

struct AppStatRow;

class TrafficPage : public StatBasePage
{
    Q_OBJECT

public:
    explicit TrafficPage(StatisticsController *ctrl = nullptr, QWidget *parent = nullptr);

    AppStatModel *appStatModel() const { return m_appStatModel; }
    TrafListModel *trafListModel() const { return m_trafListModel; }

    void selectTrafTab(int index);

protected slots:
    void onSaveWindowState(IniUser &ini) override;
    void onRestoreWindowState(IniUser &ini) override;

    void onRetranslateUi() override;

private:
    void retranslateTrafUnitNames();
    void retranslateTabBar();

    void setupUi();
    QLayout *setupHeader();
    void setupClearMenu();
    void setupEditSearch();
    void setupRefresh();
    void setupTrafUnits();
    void setupAppListView();
    void setupAppListHeader();
    void setupTabBar();
    void setupTableTraf();
    void setupTableTrafType();
    void setupTableTrafApp();
    void setupTableTrafTime();
    void setupTableTrafHeader();
    void setupSplitter();
    void setupAppInfoRow();
    void setupAppListViewChanged();

    void updateTrafType();
    void updateTrafApp(const QModelIndex &index = {});
    void updateAppListTime(const QModelIndex &index = {});

    void updateTrafUnit();
    void updateTableTrafUnit();

    int appListCurrentIndex() const;
    const AppStatRow &currentAppStatRow() const;

    int tableTrafCurrentIndex() const;

private:
    AppStatModel *m_appStatModel = nullptr;
    TrafListModel *m_trafListModel = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddProgram = nullptr;
    QAction *m_actRemoveApp = nullptr;
    QAction *m_actResetTotal = nullptr;
    QAction *m_actClearAll = nullptr;
    QAction *m_actFindApps = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QToolButton *m_btRefresh = nullptr;
    QLabel *m_traphUnits = nullptr;
    QComboBox *m_comboTrafUnit = nullptr;
    QSplitter *m_splitter = nullptr;
    TableView *m_appListView = nullptr;
    QTabBar *m_tabBar = nullptr;
    TableView *m_tableTraf = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // TRAFFICPAGE_H
