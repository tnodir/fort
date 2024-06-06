#include "ruletextparser.h"

#include <QHash>

#include <common/fortconf.h>

namespace {

RuleCharType processChar(const QChar c)
{
    if (c.isLetter()) {
        return CharNameBegin;
    }

    if (c.isDigit()) {
        return CharValueBegin;
    }

    static const QHash<char, RuleCharType> charTypeMap = {
        { '{', CharListBegin },
        { '}', CharListEnd },
        { '(', CharBracketBegin },
        { ')', CharBracketEnd },
        { '[', CharValueBegin },
        { ',', CharValueSeparator },
        { ':', CharColon },
        { '#', CharComment },
    };

    return charTypeMap.value(c.unicode(), CharNone);
}

RuleCharType processCharType(RuleCharType charType, const QChar c)
{
    if (charType == CharComment) {
        if (c == '\n') {
            return CharNone;
        }

        return CharComment;
    }

    return processChar(c);
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
    const auto charType = nextCharType();

    return false;
}

RuleCharType RuleTextParser::nextCharType()
{
    RuleCharType charType = CharNone;

    while (m_p < m_end) {
        const QChar c = *m_p++;

        charType = processCharType(charType, c);

        if (charType != CharNone) {
            return charType;
        }
    }

    return CharNone;
}
