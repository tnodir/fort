#ifndef RULETEXTPARSER_H
#define RULETEXTPARSER_H

#include <QObject>
#include <QVector>

#include <util/util_types.h>

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
    enum CharType : qint8 {
        CharNone = 0,
        CharListBegin, // {
        CharListEnd, // }
        CharBracketBegin, // (
        CharBracketEnd, // )
        CharNameBegin, // a-zA-Z
        CharName, // a-zA-Z0-9_-
        CharValueBegin, // [0-9
        CharValue, // 0-9.:-/
        CharValueSeparator, // ,
        CharColon, // :
        CharComment, // #
    };

    void setupText(const QString &text);

    RuleTextParser::CharType nextCharType();

private:
    quint8 m_exprType = 0;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    QVector<RuleExpr> m_ruleExprArray;
};

#endif // RULETEXTPARSER_H
