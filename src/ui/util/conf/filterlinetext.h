#ifndef FILTERLINETEXT_H
#define FILTERLINETEXT_H

#include <QString>

class Conn;

// A rule text line of filters by their types (FORT_RULE_FILTER_TYPE_*) with their names
class FilterLineText
{
public:
    FilterLineText(const QString &text = {});

    // The connection's filters (as by addConnAddresses()) and the opposite action
    explicit FilterLineText(const Conn &conn);

    bool isEmpty() const { return m_text.isEmpty(); }

    const QString &text() const { return m_text; }

    // Adds the filter's section with its name to the text: the values are e.g. "80, 443"
    void addFilter(qint8 type, const QString &values, bool isNot = false);

    // Adds the connection's filters to match it, in the order of FilterEditDialog's fields: the
    // direction, protocol, remote IP and port, local IP and port
    void addConnAddresses(const Conn &conn);

private:
    void addPort(qint8 type, quint16 port);

private:
    QString m_text;
};

#endif // FILTERLINETEXT_H
