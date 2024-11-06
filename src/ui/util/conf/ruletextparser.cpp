#include "ruletextparser.h"

#include <QHash>

#include <common/fortconf.h>

namespace {

const char *const extraNameChars = "_";
const char *const extraValueChars = ".-/";
const char *const extraValueEndChars = ".-/:";

int getCharIndex(const char *chars, const char c)
{
    const char *cp = strchr(chars, c);
    return cp ? (cp - chars) : -1;
}

RuleCharType processChar(const QChar c, const char *extraChars = nullptr)
{
    if (c.isLetter()) {
        return CharLetter;
    }

    if (c.isDigit()) {
        return CharDigit;
    }

    if (c == '\n') {
        return CharNewLine;
    }

    if (c.isSpace()) {
        return CharSpace;
    }

    const char c1 = c.toLatin1();

    if (extraChars && getCharIndex(extraChars, c1) >= 0) {
        return CharExtra;
    }

    static const char chars[] = "{}()[],:#!\n";
    static const RuleCharType charTypes[] = { CharListBegin, CharListEnd, CharBracketBegin,
        CharBracketEnd, CharValueBegin, CharValueEnd, CharValueSeparator, CharColon, CharComment,
        CharNot, CharNewLine };

    const int index = getCharIndex(chars, c1);

    return (index >= 0) ? charTypes[index] : CharNone;
}

RuleCharType getCharType(RuleCharType prevCharType, const QChar c, const char *extraChars = nullptr)
{
    if (prevCharType == CharComment) {
        if (c == '\n') {
            return CharNewLine;
        }

        return CharComment;
    }

    return processChar(c, extraChars);
}

}

bool RuleFilter::isTypeAddress() const
{
    return type == FORT_RULE_FILTER_TYPE_ADDRESS;
}

RuleTextParser::RuleTextParser(const QString &text, QObject *parent) : QObject(parent), m_text(text)
{
    setupCharPtr();
}

void RuleTextParser::setupCharPtr()
{
    m_p = m_text.data();
    m_end = m_p + m_text.size();
}

bool RuleTextParser::parse()
{
    return parseLines();
}

void RuleTextParser::setError(ErrorCode errorCode, const QString &errorMessage)
{
    setErrorCode(errorCode);
    setErrorMessage(errorMessage);
}

bool RuleTextParser::parseLines()
{
    const int nodeIndex = beginList(FORT_RULE_FILTER_TYPE_LIST_OR);

    for (;;) {
        if (!skipComments(CharLineBegin))
            break;

        if (!parseLine())
            break;
    }

    endList(nodeIndex);

    return !hasError();
}

bool RuleTextParser::parseLine()
{
    const int nodeIndex = beginList(FORT_RULE_FILTER_TYPE_LIST_AND);

    RuleCharTypes expectedSeparator = CharNone;

    m_ruleFilter.type = FORT_RULE_FILTER_TYPE_ADDRESS; // Default type

    for (;;) {
        if (!parseLineSection(expectedSeparator))
            break;

        if (m_ruleFilter.isLineEnd || m_ruleFilter.isListEnd)
            break;

        const bool isSectionEnd = m_ruleFilter.isSectionEnd;

        if (!checkAddFilter())
            return false;

        resetFilter();

        // Next default type, if applicable
        if (!isSectionEnd && !m_ruleFilter.hasFilterName) {
            m_ruleFilter.type = m_ruleFilter.isTypeAddress() ? FORT_RULE_FILTER_TYPE_PORT
                                                             : FORT_RULE_FILTER_TYPE_INVALID;
        }

        expectedSeparator = CharColon | CharNewLine;
    }

    endList(nodeIndex);

    return true;
}

bool RuleTextParser::parseLineSection(RuleCharTypes expectedSeparator)
{
    for (;;) {
        if (!nextCharType(CharLineBegin | expectedSeparator, CharSpace))
            return false;

        if (!processSection())
            break;
    }

    return !hasError();
}

bool RuleTextParser::processSection()
{
    if ((m_charType & (CharListBegin | CharBracketBegin | CharDigit | CharValueBegin)) != 0) {
        return processSectionBlock();
    }

    return processSectionChar();
}

bool RuleTextParser::processSectionBlock()
{
    switch (m_charType) {
    case CharListBegin: {
        processSectionList();
    } break;
    case CharBracketBegin: {
        parseBracketValues();
    } break;
    case CharDigit:
    case CharValueBegin: {
        const bool expectValueEnd = (m_charType == CharValueBegin);

        parseValue(expectValueEnd);
    } break;
    }

    return false;
}

bool RuleTextParser::processSectionChar()
{
    switch (m_charType) {
    case CharLetter: {
        return parseName();
    } break;
    case CharNot: {
        m_ruleFilter.isNot = !m_ruleFilter.isNot;
        return true;
    } break;
    case CharColon: {
        m_ruleFilter.isSectionEnd = true;
    } break;
    case CharNewLine: {
        m_ruleFilter.isLineEnd = true;
    } break;
    case CharListEnd: {
        m_ruleFilter.isListEnd = true;
        checkListEnd();
    } break;
    }

    return false;
}

void RuleTextParser::processSectionList()
{
    if (!checkListBegin())
        return;

    if (!parseLines())
        return;

    if (!m_ruleFilter.isListEnd) {
        setError(ErrorUnexpectedEndOfList, tr("Unexpected end of list"));
    }
}

bool RuleTextParser::checkListBegin()
{
    if (++m_listDepth > FORT_CONF_RULE_FILTER_DEPTH_MAX) {
        setError(ErrorListMaxDepth, tr("Max list depth exceeded: %1").arg(m_listDepth));
        return false;
    }

    return true;
}

bool RuleTextParser::checkListEnd()
{
    if (--m_listDepth < 0) {
        setError(ErrorUnexpectedSymboOfListEnd, tr("Unexpected symbol of list end"));
        return false;
    }

    return true;
}

bool RuleTextParser::parseName()
{
    const QChar *name = parsedCharPtr();

    if (!parseChars(CharName, extraNameChars)) {
        return false;
    }

    static const QHash<QString, qint8> filterTypesMap = {
        { "ip", FORT_RULE_FILTER_TYPE_ADDRESS },
        { "port", FORT_RULE_FILTER_TYPE_PORT },
        { "local_ip", FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS },
        { "local_port", FORT_RULE_FILTER_TYPE_LOCAL_PORT },
        { "proto", FORT_RULE_FILTER_TYPE_PROTOCOL },
        { "protocol", FORT_RULE_FILTER_TYPE_PROTOCOL },
        { "dir", FORT_RULE_FILTER_TYPE_DIRECTION },
        { "direction", FORT_RULE_FILTER_TYPE_DIRECTION },
        { "tcp", FORT_RULE_FILTER_TYPE_PORT_TCP },
        { "udp", FORT_RULE_FILTER_TYPE_PORT_UDP },
    };

    const QStringView nameView(name, currentCharPtr() - name);

    if (m_ruleFilter.hasFilterName) {
        setError(ErrorExtraFilterName, tr("Extra filter name: %1").arg(nameView));
        return false;
    }

    const auto nameLower = nameView.toString().toLower();

    m_ruleFilter.type = filterTypesMap.value(nameLower, FORT_RULE_FILTER_TYPE_INVALID);

    if (m_ruleFilter.type == -1) {
        setError(ErrorBadFilterName, tr("Bad filter name: %1").arg(nameView));
        return false;
    }

    m_ruleFilter.hasFilterName = true;

    return true;
}

void RuleTextParser::parseBracketValues()
{
    RuleCharTypes expectedSeparator = CharNone;

    for (;;) {
        if (!parseBracketValue(expectedSeparator))
            break;

        expectedSeparator = CharValueSeparator;
    }
}

bool RuleTextParser::parseBracketValue(RuleCharTypes expectedSeparator)
{
    resetParsedCharTypes();

    if (!nextCharType(CharValueBegin | CharValue,
                CharLineBreak | CharBracketEnd | expectedSeparator, extraValueEndChars))
        return false;

    if (hasParsedCharTypes(CharBracketEnd))
        return false;

    if (!hasParsedCharTypes(expectedSeparator)) {
        setError(ErrorUnexpectedEndOfValuesList, tr("Unexpected end of values list"));
        return false;
    }

    const bool expectValueEnd = hasParsedCharTypes(CharValueBegin);

    return parseValue(expectValueEnd);
}

bool RuleTextParser::parseValue(bool expectValueEnd)
{
    const QChar *value = parsedCharPtr();

    const char *extraChars = expectValueEnd ? extraValueEndChars : extraValueChars;

    for (;;) {
        if (!parseChars(CharLetter | CharValue, extraChars))
            return false;

        if (expectValueEnd) {
            if (m_charType != CharValueEnd) {
                setError(ErrorUnexpectedEndOfValue, tr("Unexpected end of value"));
                return false;
            }

            advanceCharPtr();

            expectValueEnd = false;
            extraChars = extraValueChars;
        } else {
            break;
        }
    }

    const QStringView valueView(value, currentCharPtr() - value);

    m_ruleFilter.addValue(valueView);

    return true;
}

bool RuleTextParser::checkAddFilter()
{
    if (!m_ruleFilter.hasValues()) {
        if (!m_ruleFilter.isSectionEnd) {
            setError(ErrorUnexpectedEndOfLineSection, tr("Unexpected end of line section"));
            return false;
        }

        return true;
    }

    if (m_ruleFilter.type == FORT_RULE_FILTER_TYPE_INVALID) {
        setError(ErrorNoFilterName, tr("No filter name"));
        return false;
    }

    addFilter();

    return true;
}

void RuleTextParser::resetFilter()
{
    m_ruleFilter.isNot = false;
    m_ruleFilter.hasFilterName = false;
    m_ruleFilter.isListEnd = false;
    m_ruleFilter.isLineEnd = false;
    m_ruleFilter.isSectionEnd = false;

    // m_ruleFilter.type is not reset

    m_ruleFilter.values.clear();
}

void RuleTextParser::addFilter()
{
    m_ruleFilters.append(m_ruleFilter);
}

int RuleTextParser::beginList(qint8 listType)
{
    const int nodeIndex = m_ruleFilters.size();

    m_ruleFilter.type = listType;

    addFilter();

    resetFilter();

    return nodeIndex;
}

void RuleTextParser::endList(int nodeIndex)
{
    const int currentIndex = m_ruleFilters.size();

    if (currentIndex == nodeIndex + 1) {
        m_ruleFilters.removeLast(); // Empty list
        return;
    }

    RuleFilter &ruleFilter = m_ruleFilters[nodeIndex];

    ruleFilter.listCount = currentIndex - nodeIndex;
}

bool RuleTextParser::skipComments(RuleCharTypes expectedCharTypes)
{
    if (!nextCharType(expectedCharTypes, CharLineBreak))
        return false;

    ungetChar();

    return true;
}

bool RuleTextParser::parseChars(
        RuleCharTypes expectedCharTypes, RuleCharTypes skipCharTypes, const char *extraChars)
{
    while (nextCharType(expectedCharTypes, skipCharTypes, extraChars)) {
        continue;
    }

    return !hasError();
}

bool RuleTextParser::nextCharType(
        RuleCharTypes expectedCharTypes, RuleCharTypes skipCharTypes, const char *extraChars)
{
    Q_ASSERT(!extraChars || (expectedCharTypes & CharExtra) != 0);

    expectedCharTypes |= skipCharTypes;

    m_charType = CharNone;

    while (!isEmpty()) {
        const QChar c = *m_p;

        m_charType = getCharType(m_charType, c, extraChars);

        if (!checkNextCharType(expectedCharTypes, c)) {
            return false;
        }

        advanceCharPtr();

        m_parsedCharTypes |= m_charType;

        if ((m_charType & skipCharTypes) == 0)
            return true;
    }

    return false;
}

bool RuleTextParser::checkNextCharType(RuleCharTypes expectedCharTypes, const QChar c)
{
    if (m_charType == CharNone) {
        setError(ErrorBadSymbol, tr("Bad symbol: %1").arg(c));
        return false;
    }

    if ((m_charType & expectedCharTypes) == 0) {
        return false;
    }

    return true;
}
