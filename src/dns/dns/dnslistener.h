#ifndef DNSLISTENER_H
#define DNSLISTENER_H

#include <QHostAddress>
#include <QObject>

#include "dnsrequest.h"

QT_FORWARD_DECLARE_CLASS(QTcpServer)
QT_FORWARD_DECLARE_CLASS(QTcpSocket)
QT_FORWARD_DECLARE_CLASS(QUdpSocket)

class DnsListener : public QObject
{
    Q_OBJECT

public:
    explicit DnsListener(QObject *parent = nullptr);
    Q_DISABLE_COPY_MOVE(DnsListener)

    // Listens on 127.0.0.1:53 and, if free, on [::1]:53
    bool listen();
    void close();

    // The socket's error of the failed listen()
    const QString &errorString() const { return m_errorString; }

    void sendResponse(const DnsClient &client, const QByteArray &response);

signals:
    void queryReceived(const DnsClient &client, const QByteArray &data);

private:
    bool listenAddress(const QHostAddress &address);
    bool listenUdp(const QHostAddress &address);
    bool listenTcp(const QHostAddress &address);

    void readDatagrams(QUdpSocket *socket);

    void acceptConnections(QTcpServer *server);
    void setupTcpConnection(QTcpSocket *socket);
    void readTcpQueries(QTcpSocket *socket);

private:
    int m_tcpConnCount = 0;

    QString m_errorString;

    QList<QUdpSocket *> m_udpSockets;
    QList<QTcpServer *> m_tcpServers;
};

#endif // DNSLISTENER_H
