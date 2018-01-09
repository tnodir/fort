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
    Q_INVOKABLE static quint32 textToIp4(const QString &text, bool *ok = nullptr);

    // Convert IPv4 address from number to text
    Q_INVOKABLE static QString ip4ToText(quint32 ip);

    // Get IPv4 address mask
    Q_INVOKABLE static int ip4Mask(quint32 ip);

    Q_INVOKABLE static QString formatDataSize(qint64 bytes, int precision = 2);

    Q_INVOKABLE static QString getHostName(const QString &address);

    Q_INVOKABLE static QStringList localIpv4Networks();

private:
    static int first0Bit(quint32 u);
    static int bitCount(quint32 u);
};

#endif // NETUTIL_H
