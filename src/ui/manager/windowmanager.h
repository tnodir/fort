#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>

#include <util/ioc/iocservice.h>

class GraphWindow;
class MainWindow;
class OptionsWindow;
class ProgramsWindow;
class StatisticsWindow;
class TrayIcon;
class ZonesWindow;

class WindowManager : public QObject, public IocService
{
    Q_OBJECT

public:
    enum TrayMessageType { MessageOptions, MessageZones };
    Q_ENUM(TrayMessageType)

    explicit WindowManager(QObject *parent = nullptr);

    MainWindow *mainWindow() const { return m_mainWindow; }
    ProgramsWindow *progWindow() const { return m_progWindow; }
    OptionsWindow *optWindow() const { return m_optWindow; }
    StatisticsWindow *connWindow() const { return m_statWindow; }
    ZonesWindow *zoneWindow() const { return m_zoneWindow; }
    GraphWindow *graphWindow() const { return m_graphWindow; }

    void setUp() override;
    void tearDown() override;

    static void showWidget(QWidget *w);

    static QFont defaultFont();

signals:
    void optWindowChanged(bool visible);
    void graphWindowChanged(bool visible);

public slots:
    void showTrayIcon();
    void closeTrayIcon();
    void showTrayMessage(
            const QString &message, WindowManager::TrayMessageType type = MessageOptions);

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

    void closeAll();

    void quitByCheckPassword();
    bool widgetVisibleByCheckPassword(QWidget *w);
    bool checkPassword();

    virtual void showErrorBox(const QString &text, const QString &title = QString());
    virtual void showInfoBox(const QString &text, const QString &title = QString());
    bool showQuestionBox(const QString &text, const QString &title = QString());
    bool showYesNoBox(const QString &text, const QString &yesText, const QString &noText,
            const QString &title = QString());

private:
    void setupMainWindow();
    void closeMainWindow();

    void setupTrayIcon();

    void setupProgramsWindow();
    void setupOptionsWindow();
    void setupZonesWindow();
    void setupGraphWindow();
    void setupStatisticsWindow();

    void onTrayMessageClicked();

    QWidget *focusWidget() const;
    static void activateModalWidget();

private:
    TrayMessageType m_lastMessageType = MessageOptions;

    TrayIcon *m_trayIcon = nullptr;

    MainWindow *m_mainWindow = nullptr;
    ProgramsWindow *m_progWindow = nullptr;
    OptionsWindow *m_optWindow = nullptr;
    StatisticsWindow *m_statWindow = nullptr;
    ZonesWindow *m_zoneWindow = nullptr;
    GraphWindow *m_graphWindow = nullptr;
};

#endif // WINDOWMANAGER_H
