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
    while (m_p < m_end) {
        processChar(*m_p++);
    }

    return false;
}

void RuleTextParser::processChar(const QChar c)
{
    if (m_skipLine) {
        m_skipLine = (c != '\n');
        return;
    }

    switch (c.unicode()) {
    case '#': {
        m_skipLine = true;
    } break;
    }
}
