#ifndef PROGRAMSWINDOW_H
#define PROGRAMSWINDOW_H

#include <form/windowtypes.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class AppInfoCache;
class AppInfoRow;
class AppListModel;
class AppListModel;
class ConfManager;
class FirewallConf;
class FortSettings;
class IniOptions;
class IniUser;
class ProgramEditDialog;
class ProgramsController;
class TableView;
class WidgetWindowStateWatcher;
class WindowManager;

struct AppRow;

class ProgramsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ProgramsWindow(QWidget *parent = nullptr);

    quint32 windowCode() const override { return WindowPrograms; }

    ProgramsController *ctrl() const { return m_ctrl; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    AppInfoCache *appInfoCache() const;
    AppListModel *appListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

    bool editProgramByPath(const QString &appPath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupEditMenu();
    void setupEditSearch();
    void setupTableApps();
    void setupTableAppsHeader();
    void setupAppInfoRow();
    void setupTableAppsChanged();

    void addNewProgram();
    void addNewWildcard();
    void editSelectedPrograms();

    void openAppEditForm(const AppRow &appRow, const QVector<qint64> &appIdList = {});
    bool checkAppEditFormOpened() const;

    void updateSelectedApps(bool blocked, bool killProcess = false);
    void deleteSelectedApps();

    int appListCurrentIndex() const;
    AppRow appListCurrentRow() const;
    QString appListCurrentPath() const;

    QStringList getAppListNames(const QVector<qint64> &appIdList, int maxCount = 4) const;

    QVector<qint64> selectedAppIdList() const;

private:
    ProgramsController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    QAction *m_actAllowApp = nullptr;
    QAction *m_actBlockApp = nullptr;
    QAction *m_actKillApp = nullptr;
    QAction *m_actAddApp = nullptr;
    QAction *m_actAddWildcard = nullptr;
    QAction *m_actEditApp = nullptr;
    QAction *m_actRemoveApp = nullptr;
    QAction *m_actReviewAlerts = nullptr;
    QAction *m_actPurgeApps = nullptr;
    QAction *m_actFindApps = nullptr;
    QPushButton *m_btEdit = nullptr;
    QToolButton *m_btAllowApp = nullptr;
    QToolButton *m_btBlockApp = nullptr;
    QToolButton *m_btRemoveApp = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QToolButton *m_btGroups = nullptr;
    QToolButton *m_btServices = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_appListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;

    ProgramEditDialog *m_formAppEdit = nullptr;
};

#endif // PROGRAMSWINDOW_H
