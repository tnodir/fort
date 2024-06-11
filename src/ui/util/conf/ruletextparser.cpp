#include "ruletextparser.h"

#include <QHash>

#include <common/fortconf.h>

namespace {

const char *const extraNameChars = "_";
const char *const extraValueChars = ".:-/]";

RuleCharType processChar(const QChar c, const char *extraChars = nullptr)
{
    if (c.isLetter()) {
        return CharLetter;
    }

    if (c.isDigit()) {
        return CharDigit;
    }

    const char c1 = c.toLatin1();

    if (extraChars && strchr(extraChars, c1)) {
        return CharExtra;
    }

    static const char chars[] = "{}()[,:#!\n";
    static const RuleCharType charTypes[] = { CharListBegin, CharListEnd, CharBracketBegin,
        CharBracketEnd, CharValueBegin, CharValueSeparator, CharColon, CharComment, CharNot,
        CharNewLine };

    const char *cp = strchr(chars, c1);

    return cp ? charTypes[cp - chars] : CharNone;
}

RuleCharType processCharType(RuleCharType charType, const QChar c, const char *extraChars = nullptr)
{
    if (charType == CharComment) {
        if (c == '\n') {
            return CharNone;
        }

        return CharComment;
    }

    return processChar(c, extraChars);
}

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
    return parseLines();
}

bool RuleTextParser::parseLines()
{
    const int listIndex = pushListNode(FORT_RULE_EXPR_LIST_OR);

    for (;;) {
        if (!parseLine())
            break;
    }

    popListNode(listIndex);

    return true;
}

bool RuleTextParser::parseLine()
{
    bool ok = false;

    const auto charType = nextCharType(CharAnyBegin);

    switch (charType) {
    case CharListBegin: {
        ok = parseLines();
    } break;
    case CharBracketBegin: {
        ok = parseBracketValues();
    } break;
    case CharLetter: {
        ok = parseName();
    } break;
    case CharNot: {
        m_isNot = !m_isNot;
    } break;
    default:
        break;
    }

    return ok;
}

bool RuleTextParser::parseName()
{
    const QChar *name = parsedCharPtr();

    while (nextCharType(CharName, extraNameChars) != CharNone) {
        continue;
    }

    if (hasError()) {
        return false;
    }

    ungetChar();

    const QStringView nameView(name, currentCharPtr() - name);
    const auto nameLower = nameView.toString().toLower();

    static const QHash<QString, qint8> exprTypesMap = {
        { "ip", FORT_RULE_EXPR_TYPE_ADDRESS },
        { "port", FORT_RULE_EXPR_TYPE_PORT },
        { "local_ip", FORT_RULE_EXPR_TYPE_LOCAL_ADDRESS },
        { "local_port", FORT_RULE_EXPR_TYPE_LOCAL_PORT },
        { "proto", FORT_RULE_EXPR_TYPE_PROTOCOL },
        { "protocol", FORT_RULE_EXPR_TYPE_PROTOCOL },
        { "dir", FORT_RULE_EXPR_TYPE_DIRECTION },
        { "direction", FORT_RULE_EXPR_TYPE_DIRECTION },
    };

    m_exprType = exprTypesMap.value(nameLower, -1);

    if (m_exprType == -1) {
        setErrorMessage(tr("Bad text: %1").arg(nameView));
        return false;
    }

    return true;
}

bool RuleTextParser::parseBracketValues()
{
    const auto endCharType = parseValues();

    return (endCharType == CharBracketEnd);
}

RuleCharType RuleTextParser::parseValues()
{
    return CharNone;
}

int RuleTextParser::pushListNode(int listType)
{
    const int listIndex = m_ruleExprArray.size();

    RuleExpr ruleExpr;
    ruleExpr.flags = FORT_RULE_EXPR_FLAG_LIST;
    ruleExpr.type = listType;

    m_ruleExprArray.append(ruleExpr);

    return listIndex;
}

void RuleTextParser::popListNode(int listIndex)
{
    const int curListIndex = m_ruleExprArray.size();

    RuleExpr &ruleExpr = m_ruleExprArray[listIndex];

    ruleExpr.listCount = curListIndex - listIndex;
}

RuleCharType RuleTextParser::nextCharType(quint32 expectedCharTypes, const char *extraChars)
{
    Q_ASSERT(!extraChars || (expectedCharTypes & CharExtra) != 0);

    const auto cp = m_p;

    RuleCharType charType = CharNone;

    while (m_p < m_end) {
        const QChar c = *m_p++;

        charType = processCharType(charType, c, extraChars);

        switch (charType) {
        case CharNone: {
            setErrorMessage(tr("Bad symbol: %1").arg(c));
            return CharNone;
        } break;
        default:
            if ((charType & expectedCharTypes) == 0) {
                if (cp == m_p) {
                    setErrorMessage(tr("Unexpected symbol: %1").arg(c));
                }
                return CharNone;
            }

            return charType;
        }
    }

    return CharNone;
}
