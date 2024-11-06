#ifndef HOMEWINDOW_H
#define HOMEWINDOW_H

#include <form/controls/formwindow.h>

class ConfManager;
class FortSettings;
class HomeController;
class HomeMainPage;
class IniUser;
class WindowManager;

class HomeWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit HomeWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowHome; }

    HomeController *ctrl() const { return m_ctrl; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

public slots:
    void selectAboutTab();

private slots:
    void onActivationChanged(bool isActive);

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QWidget *setupHeader();
    void setupPasswordButtons();
    QLayout *setupDialogButtons();

private:
    HomeController *m_ctrl = nullptr;

    HomeMainPage *m_mainPage = nullptr;
    QToolButton *m_btPasswordLock = nullptr;
    QToolButton *m_btPasswordUnlock = nullptr;
    QPushButton *m_btMenu = nullptr;

    QToolButton *m_btProfile = nullptr;
    QToolButton *m_btLogs = nullptr;
    QToolButton *m_btServiceLogs = nullptr;
    QToolButton *m_btStat = nullptr;
    QToolButton *m_btReleases = nullptr;
    QToolButton *m_btHelp = nullptr;
};

#endif // HOMEWINDOW_H
