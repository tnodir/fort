#ifndef MAINMENUBAR_H
#define MAINMENUBAR_H

#include <QMenuBar>

class MainWindow;

class MainMenuBar : public QMenuBar
{
    Q_OBJECT

public:
    explicit MainMenuBar(MainWindow *mainWindow);

private:
    void retranslateUi();

    void setupUi();
    void setupFileMenu();
    void setupToolsMenu();
    void setupWindowMenu();
    void setupHelpMenu();

    void updateFileMenu();

private:
    MainWindow *m_mainWindow = nullptr;

    QMenu *m_fileMenu = nullptr;
    QAction *m_actLock = nullptr;
    QAction *m_actUnlock = nullptr;
    QMenu *m_toolsMenu = nullptr;
    QAction *m_actReviewAlerts = nullptr;
    QAction *m_actInstallation = nullptr;
    QMenu *m_windowMenu = nullptr;
    QAction *m_actResetLayout = nullptr;
    QMenu *m_helpMenu = nullptr;
    QAction *m_actHelp = nullptr;
    QList<QAction *> m_linkActions;
    QAction *m_actCheckUpdates = nullptr;
    QAction *m_actAbout = nullptr;
};

#endif // MAINMENUBAR_H
