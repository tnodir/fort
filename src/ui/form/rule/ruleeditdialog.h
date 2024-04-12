#ifndef RULEEDITDIALOG_H
#define RULEEDITDIALOG_H

#include <QDialog>

#include <model/rulelistmodel.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ConfRuleManager;
class LineEdit;
class ListView;
class PlainTextEdit;
class Rule;
class RuleSetModel;
class RulesController;
class WindowManager;
class ZonesSelector;

class RuleEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RuleEditDialog(RulesController *ctrl, QWidget *parent = nullptr);

    RulesController *ctrl() const { return m_ctrl; }
    RuleSetModel *ruleSetModel() const { return m_ruleSetModel; }
    ConfRuleManager *confRuleManager() const;
    WindowManager *windowManager() const;

    bool isEmpty() const { return m_ruleRow.ruleId == 0; }

    void initialize(const RuleRow &ruleRow);

private:
    void initializeRuleSet();
    void initializeFocus();

    void setupController();

    void retranslateUi();
    void retranslateComboRuleType();
    void retranslateRulePlaceholderText();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupActionsLayout();
    QLayout *setupZonesLayout();
    QLayout *setupRuleSetHeaderLayout();
    void setupRuleSetView();
    QLayout *setupButtons();
    void setupRuleSetViewChanged();

    void updateRuleSetViewVisible();

    int ruleSetCurrentIndex() const;

    bool save();
    bool saveRule(Rule &rule);

    bool validateFields() const;
    void fillRule(Rule &rule) const;

    void selectPresetRuleDialog();

private:
    RulesController *m_ctrl = nullptr;
    RuleSetModel *m_ruleSetModel = nullptr;

    QLabel *m_labelEditName = nullptr;
    LineEdit *m_editName = nullptr;
    QLabel *m_labelEditNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QLabel *m_labelRuleType = nullptr;
    QComboBox *m_comboRuleType = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    QCheckBox *m_cbExclusive = nullptr;
    ZonesSelector *m_btZones = nullptr;
    PlainTextEdit *m_editRuleText = nullptr;
    QToolButton *m_btAddPresetRule = nullptr;
    QToolButton *m_btRemovePresetRule = nullptr;
    QToolButton *m_btUpPresetRule = nullptr;
    QToolButton *m_btDownPresetRule = nullptr;
    ListView *m_ruleSetView = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    RuleRow m_ruleRow;
};

#endif // RULEEDITDIALOG_H
