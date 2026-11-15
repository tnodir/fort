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

    // The queries received while paused wait in the sockets till the resume
    bool isPaused() const { return m_paused; }
    void setPaused(bool v);

    // 53 by default
    quint16 port() const { return m_port; }
    void setPort(quint16 v) { m_port = v; }

    // Listens on 127.0.0.1 and, if free, on [::1]
    bool listen();
    void close();

    // The socket's error of the failed listen()
    const QString &errorString() const { return m_errorString; }

    void sendResponse(const DnsClient &client, const QByteArray &response);

signals:
    void queryReceived(const DnsClient &client, const QByteArray &data);

private:
    bool listenAddress(const QHostAddress &address, QString &errorString);
    bool listenUdp(const QHostAddress &address, QString &errorString);
    bool listenTcp(const QHostAddress &address, QString &errorString);

    void readPausedQueries();

    void readDatagrams(QUdpSocket *socket);

    void acceptConnections(QTcpServer *server);
    void setupTcpConnection(QTcpSocket *socket);
    void readTcpQueries(QTcpSocket *socket);

private:
    bool m_paused = false;

    quint16 m_port = 53;

    int m_tcpConnCount = 0;

    QString m_errorString;

    QList<QUdpSocket *> m_udpSockets;
    QList<QTcpServer *> m_tcpServers;
};

#endif // DNSLISTENER_H
