#include "ruletextparser.h"

#include <QHash>

#include <common/fortconf.h>

namespace {

const char *const extraNameChars = "_";
const char *const extraValueChars = ".:-/]";

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

    if (c.isSpace()) {
        return CharSpace;
    }

    const char c1 = c.toLatin1();

    if (extraChars && getCharIndex(extraChars, c1) >= 0) {
        return CharExtra;
    }

    static const char chars[] = "{}()[,:#!\n";
    static const RuleCharType charTypes[] = { CharListBegin, CharListEnd, CharBracketBegin,
        CharBracketEnd, CharValueBegin, CharValueSeparator, CharColon, CharComment, CharNot,
        CharNewLine };

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
    return type == FORT_RULE_FILTER_TYPE_ADDRESS || type == FORT_RULE_FILTER_TYPE_ADDRESS_TCP
            || type == FORT_RULE_FILTER_TYPE_ADDRESS_UDP;
}

RuleTextParser::RuleTextParser(const QString &text, QObject *parent) : QObject(parent)
{
    setupText(text);
}

void RuleTextParser::setupText(const QString &text)
{
    m_p = text.data();
    m_end = m_p + text.size();
}

bool RuleTextParser::parse()
{
    parseLines();

    return !hasError();
}

void RuleTextParser::parseLines()
{
    const int nodeIndex = beginList(FORT_RULE_FILTER_TYPE_LIST_OR);

    for (;;) {
        if (!skipComments())
            break;

        if (!parseLine())
            break;
    }

    endList(nodeIndex);
}

bool RuleTextParser::skipComments()
{
    if (!nextCharType(CharAnyBegin | CharLineBreak))
        return false;

    ungetChar();

    return true;
}

bool RuleTextParser::parseLine()
{
    const int nodeIndex = beginList(FORT_RULE_FILTER_TYPE_LIST_AND);

    m_ruleFilter.type = FORT_RULE_FILTER_TYPE_ADDRESS; // default type

    for (;;) {
        if (!parseLineSection())
            break;

        if (!checkAddFilter())
            return false;

        if (!m_ruleFilter.hasFilterName) {
            // next default type, if applicable
            m_ruleFilter.type = m_ruleFilter.isTypeAddress() ? FORT_RULE_FILTER_TYPE_PORT
                                                             : FORT_RULE_FILTER_TYPE_INVALID;
        }
    }

    endList(nodeIndex);

    return true;
}

bool RuleTextParser::parseLineSection()
{
    for (;;) {
        if (!nextCharType(CharAnyBegin | CharSpace))
            return false;

        if (!processSectionChar())
            break;
    }

    return !hasError();
}

bool RuleTextParser::processSectionChar()
{
    switch (m_charType) {
    case CharListBegin: {
        processSectionLines();
    } break;
    case CharBracketBegin: {
        parseBracketValues();
    } break;
    case CharDigit:
    case CharValueBegin: {
        parseValue();
    } break;
    case CharLetter: {
        return parseName();
    } break;
    case CharNot: {
        m_ruleFilter.isNot = !m_ruleFilter.isNot;
        return true;
    } break;
    case CharColon:
    case CharNewLine: {
        m_ruleFilter.isSectionEnd = true;
    } break;
    case CharListEnd: {
        m_ruleFilter.isListEnd = true;
    } break;
    }

    return false;
}

void RuleTextParser::processSectionLines()
{
    parseLines();

    if (!m_ruleFilter.isListEnd) {
        setErrorMessage(tr("Unexpected end of list"));
    }
}

bool RuleTextParser::parseName()
{
    const QChar *name = parsedCharPtr();

    while (nextCharType(CharName, extraNameChars)) {
        continue;
    }

    if (hasError()) {
        return false;
    }

    ungetChar();

    static const QHash<QString, qint8> filterTypesMap = {
        { "ip", FORT_RULE_FILTER_TYPE_ADDRESS },
        { "port", FORT_RULE_FILTER_TYPE_PORT },
        { "local_ip", FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS },
        { "local_port", FORT_RULE_FILTER_TYPE_LOCAL_PORT },
        { "proto", FORT_RULE_FILTER_TYPE_PROTOCOL },
        { "protocol", FORT_RULE_FILTER_TYPE_PROTOCOL },
        { "dir", FORT_RULE_FILTER_TYPE_DIRECTION },
        { "direction", FORT_RULE_FILTER_TYPE_DIRECTION },
        { "tcp", FORT_RULE_FILTER_TYPE_ADDRESS_TCP },
        { "udp", FORT_RULE_FILTER_TYPE_ADDRESS_UDP },
    };

    const QStringView nameView(name, currentCharPtr() - name);

    if (m_ruleFilter.hasFilterName) {
        setErrorMessage(tr("Extra filter name: %1").arg(nameView));
        return false;
    }

    const auto nameLower = nameView.toString().toLower();

    m_ruleFilter.type = filterTypesMap.value(nameLower, FORT_RULE_FILTER_TYPE_INVALID);

    if (m_ruleFilter.type == -1) {
        setErrorMessage(tr("Bad filter name: %1").arg(nameView));
        return false;
    }

    m_ruleFilter.hasFilterName = true;

    return true;
}

void RuleTextParser::parseBracketValues()
{
    parseValue();
}

void RuleTextParser::parseValue()
{
    // TODO: implement
}

bool RuleTextParser::checkAddFilter()
{
    if (!m_ruleFilter.hasValues()) {
        if (m_ruleFilter.isSectionEnd) {
            setErrorMessage(tr("Unexpected end of line section"));
            return false;
        }

        return true;
    }

    if (m_ruleFilter.type == FORT_RULE_FILTER_TYPE_INVALID) {
        setErrorMessage(tr("No filter name"));
        return false;
    }

    addFilter();

    return true;
}

void RuleTextParser::resetFilter()
{
    m_ruleFilter.isNot = false;
    m_ruleFilter.hasFilterName = false;
    m_ruleFilter.isSectionEnd = false;
    m_ruleFilter.isListEnd = false;

    // m_ruleFilter.type is not reset

    m_ruleFilter.values.clear();
}

void RuleTextParser::addFilter()
{
    m_ruleFilterArray.append(m_ruleFilter);

    resetFilter();
}

int RuleTextParser::beginList(qint8 listType)
{
    const int nodeIndex = m_ruleFilterArray.size();

    m_ruleFilter.type = listType;

    addFilter();

    return nodeIndex;
}

void RuleTextParser::endList(int nodeIndex)
{
    const int currentIndex = m_ruleFilterArray.size();

    if (currentIndex == nodeIndex) {
        m_ruleFilterArray.removeLast(); // Empty list
        return;
    }

    RuleFilter &ruleFilter = m_ruleFilterArray[nodeIndex];

    ruleFilter.listCount = currentIndex - nodeIndex;
}

bool RuleTextParser::nextCharType(quint32 expectedCharTypes, const char *extraChars)
{
    Q_ASSERT(!extraChars || (expectedCharTypes & CharExtra) != 0);

    m_charType = CharNone;

    while (m_p < m_end) {
        const QChar c = *m_p++;

        m_charType = getCharType(m_charType, c, extraChars);

        if (!checkNextCharType(expectedCharTypes, c)) {
            m_charType = CharNone;
            return false;
        }

        if ((m_charType & (CharComment | CharSpace)) == 0)
            return true;
    }

    return false;
}

bool RuleTextParser::checkNextCharType(quint32 expectedCharTypes, const QChar c)
{
    if (m_charType == CharNone) {
        setErrorMessage(tr("Bad symbol: %1").arg(c));
        return false;
    }

    if ((m_charType & expectedCharTypes) == 0) {
        setErrorMessage(tr("Unexpected symbol: %1").arg(c));
        return false;
    }

    return true;
}
