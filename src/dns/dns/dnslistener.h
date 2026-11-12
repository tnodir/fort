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

    bool isListening() const { return !m_udpSockets.isEmpty(); }

    // Listens on 127.0.0.1:53 and [::1]:53
    bool listen();
    void close();

    void sendResponse(const DnsClient &client, const QByteArray &response);

signals:
    void queryReceived(const DnsClient &client, const QByteArray &data);

private:
    void listenUdp(const QHostAddress &address);
    void listenTcp(const QHostAddress &address);

    void readDatagrams(QUdpSocket *socket);

    void acceptConnections(QTcpServer *server);
    void readTcpQueries(QTcpSocket *socket);

private:
    QList<QUdpSocket *> m_udpSockets;
    QList<QTcpServer *> m_tcpServers;
};

#endif // DNSLISTENER_H
