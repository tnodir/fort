#ifndef PROGRAMSWINDOW_H
#define PROGRAMSWINDOW_H

#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

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

    ProgramsController *ctrl() const { return m_ctrl; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    AppInfoCache *appInfoCache() const;
    AppListModel *appListModel() const;

    void saveWindowState();
    void restoreWindowState();

    bool editProgramByPath(const QString &appPath);

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupLogOptions();
    void setupLogBlocked();
    void setupTableApps();
    void setupTableAppsHeader();
    void setupAppInfoRow();
    void setupTableAppsChanged();

    void setupAppEditForm();

    void addNewProgram();
    void editSelectedPrograms();
    void openAppEditForm(const AppRow &appRow, const QVector<qint64> &appIdList = {});

    void deleteApp(int row);

    void updateSelectedApps(bool blocked);
    void deleteSelectedApps();

    int appListCurrentIndex() const;
    AppRow appListCurrentRow() const;
    QString appListCurrentPath() const;

    QVector<qint64> selectedAppIdList() const;

private:
    ProgramsController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    QAction *m_actAllowApp = nullptr;
    QAction *m_actBlockApp = nullptr;
    QAction *m_actAddApp = nullptr;
    QAction *m_actEditApp = nullptr;
    QAction *m_actRemoveApp = nullptr;
    QAction *m_actPurgeApps = nullptr;
    QPushButton *m_btAllowApp = nullptr;
    QPushButton *m_btBlockApp = nullptr;
    QPushButton *m_btRemoveApp = nullptr;
    QPushButton *m_btEdit = nullptr;
    QPushButton *m_btServices = nullptr;
    QPushButton *m_btLogOptions = nullptr;
    QCheckBox *m_cbLogBlocked = nullptr;
    ProgramEditDialog *m_formAppEdit = nullptr;
    TableView *m_appListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // PROGRAMSWINDOW_H
