#ifndef RULESWINDOW_H
#define RULESWINDOW_H

#include <form/windowtypes.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ConfManager;
class FirewallConf;
class IniOptions;
class IniUser;
class RuleEditDialog;
class RuleListModel;
class RulesController;
class TableView;
class WidgetWindowStateWatcher;
class WindowManager;

struct RuleRow;

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
    RuleListModel *ruleListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupEditSearch();
    void setupTableRules();
    void setupTableRulesHeader();
    void setupTableRulesChanged();
    void setupRuleListModelChanged();

    void addNewRule();
    void editSelectedRule();

    void openRuleEditForm(const RuleRow &ruleRow);

    void deleteRule(int row);
    void deleteSelectedRule();

    int ruleListCurrentIndex() const;

private:
    RulesController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddRule = nullptr;
    QAction *m_actEditRule = nullptr;
    QAction *m_actRemoveRule = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_ruleListView = nullptr;

    RuleEditDialog *m_formRuleEdit = nullptr;
};

#endif // RULESWINDOW_H
