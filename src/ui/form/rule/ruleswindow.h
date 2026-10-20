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
    explicit RulesWindow(
            quint8 selectRuleTypes = 0, QWidget *parent = nullptr, Qt::WindowFlags f = {});

    WindowCode windowCode() const override { return WindowRules; }
    QString windowOverlayIconPath() const override { return ":/icons/script.png"; }

    // Bits of Rule::ruleTypeBit()
    quint8 selectRuleTypes() const { return m_selectRuleTypes; }
    bool isOpenSelectRule() const { return selectRuleTypes() != 0; }

    RulesController *ctrl() const { return m_ctrl; }
    RuleListModel *ruleListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

    static RulesWindow *showRulesDialog(quint8 selectRuleTypes, QWidget *parent);
    static RuleEditDialog *showRuleEditDialog(int ruleId, QWidget *parent);

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
    bool isRuleTypeShown(Rule::RuleType ruleType) const;
    bool isRuleTypeExpanded(Rule::RuleType ruleType) const;

    bool isSelectRuleType(Rule::RuleType ruleType) const;
    Rule::RuleType newRuleType() const;

    bool emitRuleSelected();
    bool emitRuleSelected(const QModelIndex &index);

    void addNewRule();
    void editSelectedRule();
    void duplicateSelectedRule();

    void openRuleEditForm(const RuleRow &ruleRow);
    RuleEditDialog *ruleEditForm();

    void deleteSelectedRule();

    // nullptr: no rule is selected
    const RuleRow *selectedRuleRow() const;

    QModelIndex ruleListCurrentIndex() const;

private:
    quint8 m_expandedRuleTypes = 0xFF;
    quint8 m_selectRuleTypes = 0;

    RulesController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddRule = nullptr;
    QAction *m_actEditRule = nullptr;
    QAction *m_actDuplicateRule = nullptr;
    QAction *m_actRemoveRule = nullptr;
    QAction *m_actAddDefaultPresetRules = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QToolButton *m_btOptions = nullptr;
    QToolButton *m_btStatistics = nullptr;
    QPushButton *m_btMenu = nullptr;
    TreeView *m_ruleListView = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    RuleEditDialog *m_formRuleEdit = nullptr;
};

#endif // RULESWINDOW_H
