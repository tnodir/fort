#ifndef RULEEXPR_H
#define RULEEXPR_H

#include <QObject>

#include <util/util_types.h>

class RuleExpr : public QObject
{
    Q_OBJECT

public:
    explicit RuleExpr(QObject *parent = nullptr);

    const StringViewList &viewList() const { return m_viewList; }

private:
    StringViewList m_viewList;
};

#endif // RULEEXPR_H
