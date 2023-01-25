#ifndef POLICIESWINDOW_H
#define POLICIESWINDOW_H

#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QSplitter)

class ConfManager;
class FirewallConf;
class IniOptions;
class IniUser;
class PoliciesController;
class PolicyListBox;
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
    void setupPresetSplitter();
    void setupPresetLibBox();
    void setupPresetAppBox();
    void setupGlobalSplitter();
    void setupGlobalPreBox();
    void setupGlobalPostBox();

private:
    PoliciesController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    PolicyListBox *m_presetLibBox = nullptr;
    PolicyListBox *m_presetAppBox = nullptr;
    PolicyListBox *m_globalPreBox = nullptr;
    PolicyListBox *m_globalPostBox = nullptr;
    QSplitter *m_splitter = nullptr;
    QSplitter *m_presetSplitter = nullptr;
    QSplitter *m_globalSplitter = nullptr;
};

#endif // POLICIESWINDOW_H
