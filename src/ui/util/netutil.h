#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QString>

class NetUtil : public QObject
{
    Q_OBJECT

public:
    explicit NetUtil(QObject *parent = nullptr);

    // Convert IPv4 address from text to number
    static quint32 textToIp4(const QString &text, bool *ok = 0);

    // Convert IPv4 address from number to text
    static QString ip4ToText(quint32 ip);
};

#endif // NETUTIL_H
