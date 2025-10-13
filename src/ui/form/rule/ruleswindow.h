#ifndef RULESWINDOW_H
#define RULESWINDOW_H

#include <conf/rule.h>
#include <form/controls/formwindow.h>

class RuleEditDialog;
class RuleListModel;
class RulesController;
class TreeView;

struct RuleRow;

class RulesWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit RulesWindow(Rule::RuleType ruleType = Rule::RuleNone, QWidget *parent = nullptr,
            Qt::WindowFlags f = {});

    WindowCode windowCode() const override { return WindowRules; }
    QString windowOverlayIconPath() const override { return ":/icons/script.png"; }

    Rule::RuleType ruleType() const { return m_ruleType; }
    bool isOpenSelectRule() const { return ruleType() != Rule::RuleNone; }

    RulesController *ctrl() const { return m_ctrl; }
    RuleListModel *ruleListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

    static RulesWindow *showRulesDialog(Rule::RuleType ruleType, QWidget *parent);
    static RuleEditDialog *showRuleEditDialog(int ruleId, Rule::RuleType ruleType, QWidget *parent);

signals:
    void ruleSelected(const RuleRow &ruleRow);

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupEditSearch();
    void setupTreeRules();
    void setupTreeRulesHeader();
    void setupTreeRulesExpandingChanged();
    void setupTreeRulesChanged();
    void setupRuleListModelReset();
    QLayout *setupButtons();

    void expandTreeRules();

    bool emitRuleSelected();
    bool emitRuleSelected(const QModelIndex &index);

    void addNewRule();
    void editSelectedRule();

    void openRuleEditForm(const RuleRow &ruleRow);

    void deleteSelectedRule();

    QModelIndex ruleListCurrentIndex() const;

private:
    quint8 m_expandedRuleTypes = 0xFF;

    Rule::RuleType m_ruleType = Rule::RuleNone;

    RulesController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddRule = nullptr;
    QAction *m_actEditRule = nullptr;
    QAction *m_actRemoveRule = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    TreeView *m_ruleListView = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    RuleEditDialog *m_formRuleEdit = nullptr;
};

#endif // RULESWINDOW_H
