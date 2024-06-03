#include "ruletextparser.h"

#include <common/fortconf.h>

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

RuleTextParser::CharType RuleTextParser::nextCharType()
{
    bool skipLine = false;

    while (m_p < m_end) {
        const QChar c = *m_p++;

        if (skipLine) {
            skipLine = (c != '\n');
            continue;
        }

        if (c.isLetter()) {
            return CharNameBegin;
        }

        if (c.isDigit()) {
            return CharValueBegin;
        }

        switch (c.unicode()) {
        case '{':
            return CharListBegin;
        case '}':
            return CharListEnd;
        case '(':
            return CharBracketBegin;
        case ')':
            return CharBracketEnd;
        case '[':
            return CharValueBegin;
        case ',':
            return CharValueSeparator;
        case ':':
            return CharColon;
        case '#': {
            skipLine = true;
        } break;
        }
    }

    return CharNone;
}
