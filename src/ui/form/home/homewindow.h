#ifndef HOMEWINDOW_H
#define HOMEWINDOW_H

#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QPushButton)

class ConfManager;
class HomeController;
class HomeMainPage;
class IniUser;
class WidgetWindowStateWatcher;
class WindowManager;

class HomeWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit HomeWindow(QWidget *parent = nullptr);

    HomeController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;

    void saveWindowState();
    void restoreWindowState();

public slots:
    void showMenu();
    void selectAboutTab();

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();
    QWidget *setupHeader();
    QLayout *setupLogoText();

private:
    HomeController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    HomeMainPage *m_mainPage = nullptr;
    QPushButton *m_btMenu = nullptr;
};

#endif // HOMEWINDOW_H
