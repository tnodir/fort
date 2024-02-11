#ifndef RULEEDITDIALOG_H
#define RULEEDITDIALOG_H

#include <QDialog>

#include <model/rulelistmodel.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)

class PlainTextEdit;
class Rule;
class RuleListModel;
class RulesController;
class ZonesSelector;

class RuleEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RuleEditDialog(RulesController *ctrl, QWidget *parent = nullptr);

    RulesController *ctrl() const { return m_ctrl; }
    RuleListModel *ruleListModel() const;

    bool isEmpty() const { return m_ruleRow.ruleId == 0; }

    void initialize(const RuleRow &ruleRow);

private:
    void initializeFocus();

    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupMainLayout();
    QLayout *setupFormLayout();
    QLayout *setupActionsLayout();
    void setupAdvancedOptions();
    QLayout *setupButtons();

    bool save();
    bool saveRule(Rule &rule);

    bool validateFields() const;
    void fillRule(Rule &rule) const;

private:
    RulesController *m_ctrl = nullptr;

    QLabel *m_labelEditName = nullptr;
    QLineEdit *m_editName = nullptr;
    QLabel *m_labelEditNotes = nullptr;
    PlainTextEdit *m_editNotes = nullptr;
    QCheckBox *m_cbEnabled = nullptr;
    QRadioButton *m_rbAllow = nullptr;
    QRadioButton *m_rbBlock = nullptr;
    PlainTextEdit *m_editRuleText = nullptr;
    QCheckBox *m_cbExclusive = nullptr;
    ZonesSelector *m_btZones = nullptr;
    QPushButton *m_btOptions = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    RuleRow m_ruleRow;
};

#endif // RULEEDITDIALOG_H
