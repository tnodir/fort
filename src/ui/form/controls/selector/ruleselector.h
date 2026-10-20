#ifndef RULESELECTOR_H
#define RULESELECTOR_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QToolButton)

class LineEdit;

class RuleSelector : public QWidget
{
    Q_OBJECT

public:
    explicit RuleSelector(QWidget *parent = nullptr);

    quint16 ruleId() const { return m_ruleId; }
    void setRuleId(quint16 ruleId);

    bool showRuleName() const { return m_showRuleName; }
    void setShowRuleName(bool v);

    void retranslateUi();

private:
    void updateRuleName();

    void setupUi();
    void setupController();

    void selectRuleDialog();
    void editRuleDialog();

private:
    bool m_showRuleName = true;

    quint16 m_ruleId = 0;

    LineEdit *m_editRuleName = nullptr;
    QToolButton *m_btSelectRule = nullptr;
};

#endif // RULESELECTOR_H
