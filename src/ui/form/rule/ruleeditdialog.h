#ifndef RULEEDITDIALOG_H
#define RULEEDITDIALOG_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

class RuleSet;

class RuleEditDialog : public QObject
{
    Q_OBJECT

public:
    explicit RuleEditDialog(QObject *parent = nullptr);

    RuleSet *policySet() { return m_ruleSet; }

private:
    RuleSet *m_ruleSet = nullptr;

    QGroupBox *m_gbRuleList = nullptr;
    QGroupBox *m_gbPolicyList = nullptr;
    QLineEdit *m_editName = nullptr;
};

#endif // RULEEDITDIALOG_H
