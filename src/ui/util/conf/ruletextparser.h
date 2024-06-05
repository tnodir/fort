#ifndef RULETEXTPARSER_H
#define RULETEXTPARSER_H

#include <QObject>
#include <QVector>

#include <util/util_types.h>

enum RuleCharType {
    CharNone = 0,
    CharListBegin = (1 << 0), // {
    CharListEnd = (1 << 1), // }
    CharBracketBegin = (1 << 2), // (
    CharBracketEnd = (1 << 3), // )
    CharNameBegin = (1 << 4), // a-zA-Z
    CharName = (1 << 5), // a-zA-Z0-9_-
    CharValueBegin = (1 << 6), // [0-9
    CharValue = (1 << 7), // 0-9.:-/
    CharValueSeparator = (1 << 8), // ,
    CharColon = (1 << 9), // :
    CharComment = (1 << 10), // #
};

struct RuleExpr
{
    quint8 flags = 0;
    quint8 type = 0;

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

    RuleCharType nextCharType();

private:
    quint8 m_exprType = 0;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    QVector<RuleExpr> m_ruleExprArray;
};

#endif // RULETEXTPARSER_H
