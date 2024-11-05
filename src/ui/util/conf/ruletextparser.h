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
    CharSpace = (1 << 9),
    CharNewLine = (1 << 10), // \n
    CharComment = (1 << 11), // #
    CharNot = (1 << 12), // !
    CharExtra = (1 << 13), // Name | Value
    // Complex types
    CharAnyBegin =
            (CharListBegin | CharBracketBegin | CharLetter | CharDigit | CharValueBegin | CharNot),
    CharName = (CharLetter | CharExtra), // a-zA-Z_
    CharValue = (CharDigit | CharValueBegin | CharExtra), // 0-9.:-/]
    CharLineBreak = (CharSpace | CharNewLine | CharComment),
};

struct RuleFilter
{
    bool isTypeAddress() const;
    bool hasValues() const { return !values.isEmpty(); }

    bool isNot : 1 = false;
    bool hasFilterName : 1 = false;
    bool isSectionEnd : 1 = false;
    bool isListEnd : 1 = false;

    qint8 type = 0;

    quint16 listCount = 0;

    StringViewList values;
};

class RuleTextParser : public QObject
{
    Q_OBJECT

public:
    explicit RuleTextParser(const QString &text, QObject *parent = nullptr);

    QString errorMessage() const { return m_errorMessage; }

    bool hasError() const { return !errorMessage().isEmpty(); }

    const QVector<RuleFilter> &ruleFilterArray() const { return m_ruleFilterArray; }

    bool parse();

private:
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }

    void setupText(const QString &text);

    void parseLines();
    bool skipComments();
    bool parseLine();
    bool parseLineSection();
    bool processSectionChar();
    void processSectionLines();

    bool parseName();

    void parseBracketValues();
    void parseValue();

    bool checkAddFilter();

    void resetFilter();

    void addFilter();
    int beginList(qint8 listType);
    void endList(int nodeIndex);

    void ungetChar() { --m_p; }

    const QChar *currentCharPtr() const { return m_p; }
    const QChar *parsedCharPtr() const { return m_p - 1; }

    RuleFilter &listNode(int listIndex) { return m_ruleFilterArray[listIndex]; }

    bool nextCharType(quint32 expectedCharTypes, const char *extraChars = nullptr);
    bool checkNextCharType(quint32 expectedCharTypes, const QChar c);

private:
    RuleCharType m_charType = CharNone;
    RuleFilter m_ruleFilter;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    QString m_errorMessage;

    QVector<RuleFilter> m_ruleFilterArray;
};

#endif // RULETEXTPARSER_H
