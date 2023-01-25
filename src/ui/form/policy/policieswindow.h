#ifndef POLICIESWINDOW_H
#define POLICIESWINDOW_H

#include <util/window/widgetwindow.h>

class ConfManager;
class FirewallConf;
class IniOptions;
class IniUser;
class PoliciesController;
class TableView;
class WidgetWindowStateWatcher;
class WindowManager;

class PoliciesWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit PoliciesWindow(QWidget *parent = nullptr);

    PoliciesController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;

    void saveWindowState();
    void restoreWindowState();

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();

private:
    PoliciesController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;
};

#endif // POLICIESWINDOW_H
