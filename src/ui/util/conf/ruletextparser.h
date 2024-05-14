#ifndef RULETEXTPARSER_H
#define RULETEXTPARSER_H

#include <QObject>

#include <util/util_types.h>

class RuleTextParser : public QObject
{
    Q_OBJECT

public:
    explicit RuleTextParser(const QString &text, QObject *parent = nullptr);

    const StringViewList &viewList() const { return m_viewList; }

    bool parse();

private:
    void setupText(const QString &text);

    void processChar(const QChar c);

private:
    bool m_skipLine : 1 = false;

    const QChar *m_p = nullptr;
    const QChar *m_end = nullptr;

    StringViewList m_viewList;
};

#endif // RULETEXTPARSER_H
