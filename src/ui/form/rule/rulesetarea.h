#ifndef RULESETAREA_H
#define RULESETAREA_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QToolButton)

class ListView;
class Rule;
class RuleSetModel;

class RuleSetArea : public QWidget
{
    Q_OBJECT

public:
    explicit RuleSetArea(quint8 selectRuleTypes, QWidget *parent = nullptr);

    RuleSetModel *ruleSetModel() const { return m_ruleSetModel; }

    void initialize(const Rule &rule, const QStringList &ruleSetNames);

    void retranslateUi();

private:
    void setupUi();
    QLayout *setupHeaderLayout();
    void setupRuleSetView();
    void setupRuleSetViewChanged();

    int ruleSetCurrentIndex() const;

    void selectRuleDialog();
    void editCurrentRuleDialog();

private:
    quint8 m_selectRuleTypes = 0;

    RuleSetModel *m_ruleSetModel = nullptr;

    QToolButton *m_btAddRule = nullptr;
    QToolButton *m_btRemoveRule = nullptr;
    QToolButton *m_btEditRule = nullptr;
    QToolButton *m_btUpRule = nullptr;
    QToolButton *m_btDownRule = nullptr;
    ListView *m_ruleSetView = nullptr;
};

#endif // RULESETAREA_H
