#ifndef FORTMANAGER_H
#define FORTMANAGER_H

#include <QObject>

#include "util/classhelpers.h"

class AppListModel;
class FirewallConf;
class GraphWindow;
class MainWindow;
class OptionsWindow;
class ProgramsWindow;
class StatisticsWindow;
class TrayIcon;
class ZoneListModel;
class ZonesWindow;

class FortManager : public QObject
{
    Q_OBJECT

public:
    enum TrayMessageType { MessageOptions, MessageZones };
    Q_ENUM(TrayMessageType)

    explicit FortManager(QObject *parent = nullptr);
    ~FortManager() override;
    CLASS_DELETE_COPY_MOVE(FortManager)

    MainWindow *mainWindow() const { return m_mainWindow; }
    ProgramsWindow *progWindow() const { return m_progWindow; }
    OptionsWindow *optWindow() const { return m_optWindow; }
    StatisticsWindow *connWindow() const { return m_statWindow; }
    ZonesWindow *zoneWindow() const { return m_zoneWindow; }
    GraphWindow *graphWindow() const { return m_graphWindow; }

    AppListModel *appListModel() const { return m_appListModel; }
    ZoneListModel *zoneListModel() const { return m_zoneListModel; }

    bool checkRunningInstance();

    void initialize();

signals:
    void optWindowChanged(bool visible);
    void graphWindowChanged(bool visible);

public slots:
    bool installDriver();
    bool removeDriver();

    void show();

    void showTrayIcon();
    void closeTrayIcon();
    void showTrayMessage(
            const QString &message, FortManager::TrayMessageType type = MessageOptions);

    void showProgramsWindow();
    void closeProgramsWindow();

    bool showProgramEditForm(const QString &appPath);

    void showOptionsWindow();
    void closeOptionsWindow();
    void reloadOptionsWindow(const QString &reason);

    void showStatisticsWindow();
    void closeStatisticsWindow();

    void showZonesWindow();
    void closeZonesWindow();

    void showGraphWindow();
    void closeGraphWindow(bool wasVisible = false);
    void switchGraphWindow();

    void processRestartRequired();

    void quitByCheckPassword();

    bool checkPassword();

    void showErrorBox(const QString &text, const QString &title = QString());
    void showInfoBox(const QString &text, const QString &title = QString());
    bool showQuestionBox(const QString &text, const QString &title = QString());
    bool showYesNoBox(const QString &text, const QString &yesText, const QString &noText,
            const QString &title = QString());

    static void setupResources();

private:
    void setupThreadPool();

    void setupLogger();
    void updateLogger();

    void createManagers();
    void deleteManagers();

    bool setupDriver();
    void closeDriver();

    void setupEnvManager();
    void setupConfManager();
    void setupQuotaManager();
    void setupTaskManager();

    void setupModels();

    void setupTranslationManager();

    void setupMainWindow();
    void closeMainWindow();

    void setupTrayIcon();

    void setupProgramsWindow();
    void setupOptionsWindow();
    void setupZonesWindow();
    void setupGraphWindow();
    void setupStatisticsWindow();

    void closeUi();

    void loadConf();

    bool updateDriverConf(bool onlyFlags = false);

    void updateLogManager(bool active);
    void updateStatManager(FirewallConf *conf);

    void onTrayMessageClicked();

    QWidget *focusWidget() const;
    static void activateModalWidget();

private:
    bool m_initialized : 1;

    TrayMessageType m_lastMessageType = MessageOptions;

    void *m_instanceMutex = nullptr;

    TrayIcon *m_trayIcon = nullptr;

    MainWindow *m_mainWindow = nullptr;
    ProgramsWindow *m_progWindow = nullptr;
    OptionsWindow *m_optWindow = nullptr;
    StatisticsWindow *m_statWindow = nullptr;
    ZonesWindow *m_zoneWindow = nullptr;
    GraphWindow *m_graphWindow = nullptr;

    AppListModel *m_appListModel = nullptr;
    ZoneListModel *m_zoneListModel = nullptr;
};

#endif // FORTMANAGER_H
