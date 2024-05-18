#ifndef RULETEXTPARSER_H
#define RULETEXTPARSER_H

#include <QObject>
#include <QVector>

#include <util/util_types.h>

struct RuleExpr
{
    bool cond_or : 1 = false;
    bool cond_not : 1 = false;

    enum ExprType : qint8 {
        ExprNone = 0,
        ExprList,
        ExprAddress,
        ExprLocalAddress,
        ExprPort,
        ExprLocalPort,
        ExprDirection,
        ExprProtocol,
    };

    ExprType exprType = ExprNone;

    quint16 listIndex = 0;
    quint16 listCount = 0;

    StringViewList viewList;
};

class RuleTextParser : public QObject
{
    Q_OBJECT

public:
    explicit RuleTextParser(const QString &text, QObject *parent = nullptr);

    RuleExpr::ExprType exprType() const { return m_exprType; }

    const QVector<RuleExpr> &ruleExprList() const { return m_ruleExprList; }

    bool parse();

private:
    void setupText(const QString &text);

    void processChar(const QChar c);

private:
    bool m_skipLine : 1 = false;

    RuleExpr::ExprType m_exprType = RuleExpr::ExprNone;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    QVector<RuleExpr> m_ruleExprList;
};

#endif // RULETEXTPARSER_H
