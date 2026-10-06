#ifndef TRAFFICWINDOW_H
#define TRAFFICWINDOW_H

#include <QModelIndex>

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTabBar)
QT_FORWARD_DECLARE_CLASS(QTableView)

class AppInfoRow;
class AppStatModel;
class TableView;
class TrafListModel;
class TrafficController;

struct AppStatRow;

class TrafficWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit TrafficWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowTraffic; }
    QString windowOverlayIconPath() const override { return ":/icons/chart_bar.png"; }

    TrafficController *ctrl() const { return m_ctrl; }
    AppStatModel *appStatModel() const { return m_appStatModel; }
    TrafListModel *trafListModel() const { return m_trafListModel; }

    void selectTrafTab(int index);

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();
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

    void saveTrafTabIndex(int tabIndex);

private:
    TrafficController *m_ctrl = nullptr;
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
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    QSplitter *m_splitter = nullptr;
    TableView *m_appListView = nullptr;
    QTabBar *m_tabBar = nullptr;
    TableView *m_tableTraf = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // TRAFFICWINDOW_H
