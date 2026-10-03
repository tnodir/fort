#include "textmatcher.h"

void TextMatcher::setFilter(const QString &filter)
{
    m_regexp = {};
    m_words.clear();

    if (filter.startsWith('/')) {
        const QString pattern = filter.mid(1);
        if (!pattern.isEmpty()) {
            m_regexp = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        }
    } else {
        m_words = filter.split(' ', Qt::SkipEmptyParts);
    }
}

bool TextMatcher::isMatched(const QString &text) const
{
    if (!m_regexp.pattern().isEmpty())
        return m_regexp.match(text).hasMatch();

    for (const QString &word : m_words) {
        if (!text.contains(word, Qt::CaseInsensitive))
            return false;
    }

    return true;
}
