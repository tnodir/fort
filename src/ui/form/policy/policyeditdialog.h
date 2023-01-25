#ifndef POLICYEDITDIALOG_H
#define POLICYEDITDIALOG_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

class PolicySet;

class PolicyEditDialog : public QObject
{
    Q_OBJECT

public:
    explicit PolicyEditDialog(QObject *parent = nullptr);

    PolicySet *policySet() { return m_policySet; }

private:
    PolicySet *m_policySet = nullptr;

    QGroupBox *m_gbRuleList = nullptr;
    QGroupBox *m_gbPolicyList = nullptr;
    QLineEdit *m_editName = nullptr;
};

#endif // POLICYEDITDIALOG_H
