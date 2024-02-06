#ifndef RULESWINDOW_H
#define RULESWINDOW_H

#include <form/windowtypes.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QSplitter)

class ConfManager;
class FirewallConf;
class IniOptions;
class IniUser;
class RulesController;
class RuleListBox;
class TableView;
class WidgetWindowStateWatcher;
class WindowManager;

class RulesWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit RulesWindow(QWidget *parent = nullptr);

    quint32 windowCode() const override { return WindowRules; }

    RulesController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();
    void setupPresetSplitter();
    void setupPresetLibBox();
    void setupPresetAppBox();
    void setupGlobalSplitter();
    void setupGlobalPreBox();
    void setupGlobalPostBox();

private:
    RulesController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    RuleListBox *m_presetLibBox = nullptr;
    RuleListBox *m_presetAppBox = nullptr;
    RuleListBox *m_globalPreBox = nullptr;
    RuleListBox *m_globalPostBox = nullptr;
    QSplitter *m_splitter = nullptr;
    QSplitter *m_presetSplitter = nullptr;
    QSplitter *m_globalSplitter = nullptr;
};

#endif // RULESWINDOW_H
