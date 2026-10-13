#ifndef TERMINATINGRULESELECTOR_H
#define TERMINATINGRULESELECTOR_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)

class Rule;

class TerminatingRuleSelector : public QWidget
{
    Q_OBJECT

public:
    explicit TerminatingRuleSelector(QWidget *parent = nullptr);

    // By the Rule's Terminating flags
    void initialize(const Rule &rule);
    void fillRule(Rule &rule) const;

    bool terminate() const;
    void setTerminate(bool v);

    bool terminateBlocked() const; // Block or Drop
    bool terminateDrop() const;
    void setTerminateAction(bool blocked, bool drop);

    bool terminateAlert() const;
    void setTerminateAlert(bool v);

    void retranslateUi();

private:
    void setupUi();
    void setupCbTerminate();

private:
    QCheckBox *m_cbTerminate = nullptr;
    QComboBox *m_comboAction = nullptr;
    QCheckBox *m_cbAlert = nullptr;
};

#endif // TERMINATINGRULESELECTOR_H
