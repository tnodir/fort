#ifndef RULESETMODEL_H
#define RULESETMODEL_H

#include <conf/rule.h>
#include <util/model/stringlistmodel.h>

class ConfRuleManager;

struct RuleRow;

class RuleSetModel : public StringListModel
{
    Q_OBJECT

public:
    explicit RuleSetModel(QObject *parent = nullptr);

    bool edited() const { return m_edited; }
    void setEdited(bool v);

    const RuleSetList &ruleSet() const { return m_ruleSet; }

    ConfRuleManager *confRuleManager() const;

    void initialize(const RuleRow &ruleRow, const QStringList &ruleSetNames);

signals:
    void rowCountChanged();

public slots:
    void addRule(const RuleRow &ruleRow);

    void remove(int row = -1) override;

    void move(int fromRow, int toRow);
    inline void moveUp(int row) { move(row, row - 1); }
    inline void moveDown(int row) { move(row, row + 1); }

private:
    bool m_edited = false;

    Rule::RuleType m_ruleType = Rule::AppRule;
    int m_ruleId = 0;

    RuleSetList m_ruleSet;
};

#endif // RULESETMODEL_H
