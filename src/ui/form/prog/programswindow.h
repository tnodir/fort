#ifndef PROGRAMSWINDOW_H
#define PROGRAMSWINDOW_H

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QHeaderView)
QT_FORWARD_DECLARE_CLASS(QMenu)

class App;
class AppInfoRow;
class AppListModel;
class ProgramEditController;
class ProgramEditDialog;
class ProgramsController;
class TableView;

struct AppRow;

class ProgramsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit ProgramsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowPrograms; }
    QString windowOverlayIconPath() const override { return ":/icons/application.png"; }

    ProgramsController *ctrl() const { return m_ctrl; }
    AppListModel *appListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

    bool editProgramByPath(const QString &appPath);

    static void openProgramByPath(const QString &appPath, qint64 appId, FormWindow *parentForm);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupController();

    void retranslateUi();
    void retranslateTimerMenuActions();

    void setupUi();
    QLayout *setupHeader();
    QLayout *setupToolbarButtonsLayout();
    void setupEditMenu();
    void setupTimerMenuActions();
    void setupEditSearch();
    void setupFilter();
    void setupFilterCheckBoxes();
    void setupFilterClear();
    QLayout *setupSortStatesLayout();
    void setupTableApps();
    void setupTableAppsHeader();
    void setupAppInfoRow();
    void setupTableAppsChanged();

    void showTableAppsHeaderMenu(const QPoint &pos);
    void setupTableAppsHeaderMenuColumns(QMenu *menu, QHeaderView *header);

    void onSortStateClicked(int sortState);
    void updateSortStateCounts();

    void addNewProgram();
    void addNewWildcard();
    void editSelectedPrograms();

    bool checkAppEditFormOpened() const;
    void openAppEditForm(const App &app, const QVector<qint64> &appIdList = {});

    struct OpenAppEditDialogArgs
    {
        const App &app;
        const QVector<qint64> &appIdList;
        FormWindow *parentForm;
        ProgramEditDialog *&dialog;
    };

    static void openAppEditDialog(const OpenAppEditDialogArgs &a);

    void updateSelectedApps(bool blocked, bool killProcess = false);
    void deleteSelectedApps();

    void deleteAlertedApps();
    void clearAlerts();

    int appListCurrentIndex() const;
    const AppRow &appListCurrentRow() const;
    QString appListCurrentPath() const;

    QStringList getAppListNames(const QVector<qint64> &appIdList, int maxCount = 4) const;

    QVector<qint64> selectedAppIdList() const;

private:
    ProgramsController *m_ctrl = nullptr;

    QMenu *m_timerMenu = nullptr;
    QActionGroup *m_timerMenuActions = nullptr;
    QAction *m_actAllowApp = nullptr;
    QAction *m_actBlockApp = nullptr;
    QAction *m_actKillApp = nullptr;
    QAction *m_actAddApp = nullptr;
    QAction *m_actAddWildcard = nullptr;
    QAction *m_actEditApp = nullptr;
    QAction *m_actRemoveApp = nullptr;
    QAction *m_actAppCopyPath = nullptr;
    QAction *m_actAppOpenFolder = nullptr;
    QAction *m_actReviewAlerts = nullptr;
    QAction *m_actDeleteAlertedApps = nullptr;
    QAction *m_actClearAlerts = nullptr;
    QAction *m_actPurgeApps = nullptr;
    QAction *m_actFindApps = nullptr;
    QPushButton *m_btEdit = nullptr;
    QToolButton *m_btAllowApp = nullptr;
    QToolButton *m_btBlockApp = nullptr;
    QToolButton *m_btRemoveApp = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QPushButton *m_btFilter = nullptr;
    QToolButton *m_btClearFilter = nullptr;
    QCheckBox *m_cbFilterAlerted = nullptr;
    QCheckBox *m_cbFilterWildcard = nullptr;
    QCheckBox *m_cbFilterParked = nullptr;
    QCheckBox *m_cbFilterKillProcess = nullptr;
    QToolButton *m_btSortAllowed = nullptr;
    QToolButton *m_btSortBlocked = nullptr;
    QToolButton *m_btSortAlerted = nullptr;
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_appListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;

    ProgramEditDialog *m_formAppEdit = nullptr;
};

#endif // PROGRAMSWINDOW_H
