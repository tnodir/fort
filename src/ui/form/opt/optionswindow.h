#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include <util/window/widgetwindow.h>

class ConfManager;
class FirewallConf;
class FortManager;
class IniOptions;
class IniUser;
class OptMainPage;
class OptionsController;
class WidgetWindowStateWatcher;

class OptionsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(QWidget *parent = nullptr);

    OptionsController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;

    void cancelChanges();

    void saveWindowState();
    void restoreWindowState();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();

private:
    OptionsController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    OptMainPage *m_mainPage = nullptr;
};

#endif // OPTIONSWINDOW_H
