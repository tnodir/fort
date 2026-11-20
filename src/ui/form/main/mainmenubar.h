#ifndef MAINMENUBAR_H
#define MAINMENUBAR_H

#include <QMenuBar>

class MainWindow;

class MainMenuBar : public QMenuBar
{
    Q_OBJECT

public:
    explicit MainMenuBar(MainWindow *mainWindow);

    // The Backup is disabled while the password is required
    void setLocked(bool locked);

private:
    void retranslateUi();

    void setupUi();
    void setupFileMenu();
    void setupBackupMenu();
    void setupToolsMenu();
    void setupWindowMenu();
    void setupHelpMenu();

private:
    MainWindow *m_mainWindow = nullptr;

    QMenu *m_fileMenu = nullptr;
    QMenu *m_backupMenu = nullptr;
    QAction *m_actExport = nullptr;
    QAction *m_actImport = nullptr;
    QAction *m_actImportApps = nullptr;
    QMenu *m_toolsMenu = nullptr;
    QAction *m_actReviewAlerts = nullptr;
    QMenu *m_windowMenu = nullptr;
    QAction *m_actInstallation = nullptr;
    QAction *m_actResetLayout = nullptr;
    QMenu *m_helpMenu = nullptr;
    QAction *m_actHelp = nullptr;
    QList<QAction *> m_linkActions;
    QAction *m_actCheckUpdates = nullptr;
    QAction *m_actAbout = nullptr;
};

#endif // MAINMENUBAR_H
