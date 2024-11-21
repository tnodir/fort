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
    CharValueEnd = (1 << 7), // ]
    CharValueSeparator = (1 << 8), // ,
    CharColon = (1 << 9), // :
    CharSpace = (1 << 10),
    CharNewLine = (1 << 11), // \n
    CharComment = (1 << 12), // #
    CharNot = (1 << 13), // !
    CharExtra = (1 << 14), // Name | Value
    // Complex types
    CharLineBegin = (CharListBegin | CharListEnd | CharBracketBegin | CharLetter | CharDigit
            | CharValueBegin | CharNot),
    CharName = (CharLetter | CharExtra), // a-zA-Z_
    CharValue = (CharDigit | CharExtra), // 0-9._-/:
    CharSpaceComment = (CharSpace | CharComment),
    CharLineBreak = (CharSpaceComment | CharNewLine),
};

class RuleFilter
{
public:
    bool isTypeAddress() const;
    bool isTypeList() const;
    bool hasValues() const { return !values.isEmpty(); }

    void addValue(const QStringView v) { values.append(v); }

public:
    bool isNot : 1 = false;
    bool hasFilterName : 1 = false;
    bool isListEnd : 1 = false;
    bool isLineEnd : 1 = false;
    bool isSectionEnd : 1 = false;

    qint8 type = 0;

    quint16 filterListCount = 0;

    StringViewList values;
};

class RuleTextParser : public QObject
{
    Q_OBJECT

public:
    enum ErrorCode : quint16 {
        ErrorNone = 0,
        ErrorUnexpectedStartOfLine,
        ErrorUnexpectedEndOfList,
        ErrorUnexpectedEndOfValuesList,
        ErrorUnexpectedSymboOfListEnd,
        ErrorUnexpectedEndOfValue,
        ErrorListMaxDepth,
        ErrorExtraFilterName,
        ErrorBadFilterName,
        ErrorNoFilterName,
        ErrorBadSymbol,
    };
    Q_ENUM(ErrorCode)

    explicit RuleTextParser(const QString &text, QObject *parent = nullptr);

    ErrorCode errorCode() const { return m_errorCode; }
    const QString &errorMessage() const { return m_errorMessage; }

    bool hasError() const { return errorCode() != ErrorNone; }

    const QVector<RuleFilter> &ruleFilters() const { return m_ruleFilters; }

    bool parse();

private:
    void setErrorCode(ErrorCode v) { m_errorCode = v; }
    void setErrorMessage(const QString &v) { m_errorMessage = v; }

    void setError(ErrorCode errorCode, const QString &errorMessage);

    void setupCharPtr();

    bool parseLines();
    bool parseLineComments();
    bool parseLine();
    bool processSectionFilter();
    bool parseLineSection(RuleCharTypes expectedSeparator);
    bool parseSection();
    bool parseSectionBlock();
    bool parseSectionChar();
    void parseSectionList();
    void checkSectionListEnd();
    bool checkListBegin();
    bool checkListEnd();

    bool parseName();
    bool parseNot();

    void parseBracketValues();
    bool parseBracketValue(RuleCharTypes expectedSeparator);
    bool checkBracketValuesSeparator(RuleCharTypes expectedSeparator);
    bool checkBracketValueEnd();
    bool parseValue(bool expectValueEnd);
    bool parseValueChars(bool &expectValueEnd);
    bool checkValueEnd(bool &expectValueEnd);

    bool checkAddFilter();

    void resetFilter();

    void addFilter();
    int beginList(qint8 listType);
    void endList(int filterIndex);

    void resetParsedCharTypes() { m_parsedCharTypes = CharNone; }
    bool hasParsedCharTypes(RuleCharTypes v) { return v == 0 || (m_parsedCharTypes & v) != 0; }

    void ungetChar() { --m_p; }
    void advanceCharPtr() { ++m_p; }

    const QChar *currentCharPtr() const { return m_p; }
    const QChar *parsedCharPtr() const { return m_p - 1; }

    bool isEmpty() const { return m_p >= m_end; }

    RuleFilter &listNode(int filterIndex) { return m_ruleFilters[filterIndex]; }

    bool skipComments(RuleCharTypes expectedCharTypes);

    inline bool parseChars(RuleCharTypes expectedCharTypes, const char *extraChars = nullptr)
    {
        return parseChars(expectedCharTypes, CharNone, extraChars);
    }

    bool parseChars(RuleCharTypes expectedCharTypes, RuleCharTypes skipCharTypes,
            const char *extraChars = nullptr);

    bool nextCharType(RuleCharTypes expectedCharTypes, RuleCharTypes skipCharTypes,
            const char *extraChars = nullptr);

    bool checkNextCharType(RuleCharTypes expectedCharTypes, const QChar c);

    void returnToCharType(RuleCharTypes expectedCharTypes);

private:
    qint8 m_listDepth = 0;
    ErrorCode m_errorCode = ErrorNone;
    RuleCharType m_charType = CharNone;
    RuleCharTypes m_parsedCharTypes = CharNone;
    RuleFilter m_ruleFilter;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    QString m_text;
    QString m_errorMessage;

    QVector<RuleFilter> m_ruleFilters;
};

#endif // RULETEXTPARSER_H
