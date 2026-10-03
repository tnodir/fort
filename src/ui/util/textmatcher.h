#ifndef TEXTMATCHER_H
#define TEXTMATCHER_H

#include <QRegularExpression>
#include <QStringList>

class TextMatcher
{
public:
    /* The space separated words to contain or the regular expression after '/' */
    void setFilter(const QString &filter);

    bool isEmpty() const { return m_words.isEmpty() && m_regexp.pattern().isEmpty(); }

    bool isMatched(const QString &text) const;

private:
    QRegularExpression m_regexp;
    QStringList m_words;
};

#endif // TEXTMATCHER_H
