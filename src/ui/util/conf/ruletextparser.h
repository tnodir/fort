#ifndef RULETEXTPARSER_H
#define RULETEXTPARSER_H

#include <QObject>
#include <QVector>

#include <util/util_types.h>

struct RuleExpr
{
    quint8 flags = 0;

    quint16 listIndex = 0;
    quint16 listCount = 0;

    StringViewList viewList;
};

class RuleTextParser : public QObject
{
    Q_OBJECT

public:
    explicit RuleTextParser(const QString &text, QObject *parent = nullptr);

    const QVector<RuleExpr> &ruleExprArray() const { return m_ruleExprArray; }

    bool parse();

private:
    void setupText(const QString &text);

    void processChar(const QChar c);

private:
    bool m_skipLine : 1 = false;

    quint8 m_exprType = 0;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    QVector<RuleExpr> m_ruleExprArray;
};

#endif // RULETEXTPARSER_H
