#ifndef RULETEXTPARSER_H
#define RULETEXTPARSER_H

#include <QObject>
#include <QVector>

#include <util/util_types.h>

using RuleCharTypes = quint16;

enum RuleCharType : RuleCharTypes {
    CharNone = 0,
    CharListBegin = (1 << 0), // {
    CharListEnd = (1 << 1), // }
    CharBracketBegin = (1 << 2), // (
    CharBracketEnd = (1 << 3), // )
    CharLetter = (1 << 4), // a-zA-Z
    CharDigit = (1 << 5), // 0-9
    CharValueBegin = (1 << 6), // [
    CharValueSeparator = (1 << 7), // ,
    CharColon = (1 << 8), // :
    CharComment = (1 << 9), // #
    CharNot = (1 << 10), // !
    CharExtra = (1 << 11), // Name | Value
    CharNewLine = (1 << 12), // \n
    CharAnyBegin =
            (CharListBegin | CharBracketBegin | CharLetter | CharDigit | CharValueBegin | CharNot),
    CharName = (CharLetter | CharExtra), // a-zA-Z_
    CharValue = (CharDigit | CharValueBegin | CharExtra), // 0-9.:-/]
    CharAny = RuleCharTypes(-1),
};

struct RuleExpr
{
    quint8 flags = 0;
    quint8 type = 0;

    quint16 listCount = 0;

    StringViewList viewList;
};

class RuleTextParser : public QObject
{
    Q_OBJECT

public:
    explicit RuleTextParser(const QString &text, QObject *parent = nullptr);

    QString errorMessage() const { return m_errorMessage; }

    bool hasError() const { return !errorMessage().isEmpty(); }

    const QVector<RuleExpr> &ruleExprArray() const { return m_ruleExprArray; }

    bool parse();

private:
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }

    void setupText(const QString &text);

    bool parseLines();
    bool parseLine();

    bool parseName();

    bool parseBracketValues();
    RuleCharType parseValues();

    int pushListNode(int listType);
    void popListNode(int listIndex);

    void ungetChar() { --m_p; }

    const QChar *currentCharPtr() const { return m_p; }
    const QChar *parsedCharPtr() const { return m_p - 1; }

    RuleExpr &listNode(int listIndex) { return m_ruleExprArray[listIndex]; }

    RuleCharType nextCharType(quint32 expectedCharTypes, const char *extraChars = nullptr);
    bool checkNextCharType(
            quint32 expectedCharTypes, RuleCharType &charType, const QChar *cp, const QChar c);

private:
    bool m_isNot = false;

    quint8 m_exprType = 0;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    QString m_errorMessage;

    QVector<RuleExpr> m_ruleExprArray;
};

#endif // RULETEXTPARSER_H
