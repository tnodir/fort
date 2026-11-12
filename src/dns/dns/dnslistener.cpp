#include "dnslistener.h"

#include <QLoggingCategory>
#include <QNetworkDatagram>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QtEndian>

namespace {

const QLoggingCategory LC("dns.listener");

inline constexpr quint16 dnsPort = 53;

// A TCP message is prefixed by its size
bool readTcpQuery(QTcpSocket *socket, QByteArray &data)
{
    quint16 size;
    if (socket->peek((char *) &size, sizeof(size)) != sizeof(size))
        return false;

    size = qFromBigEndian(size);

    if (socket->bytesAvailable() < qint64(sizeof(size)) + size)
        return false;

    socket->skip(sizeof(size));
    data = socket->read(size);

    return true;
}

void writeTcpResponse(QAbstractSocket *socket, const QByteArray &response)
{
    const quint16 size = qToBigEndian(quint16(response.size()));

    socket->write((const char *) &size, sizeof(size));
    socket->write(response);
}

// Deletes the listener on error
template<typename T>
bool appendListener(QList<T *> &list, T *listener, bool ok, const QHostAddress &address)
{
    if (!ok) {
        qCWarning(LC) << "Listen error:" << listener->metaObject()->className() << address
                      << listener->errorString();

        delete listener;
        return false;
    }

    list.append(listener);

    return true;
}

}

DnsListener::DnsListener(QObject *parent) : QObject(parent) { }

bool DnsListener::listen()
{
    close();

    // The NRPT rule sends the queries to 127.0.0.1
    if (!listenAddress(QHostAddress::LocalHost)) {
        close();
        return false;
    }

    listenAddress(QHostAddress::LocalHostIPv6);

    return true;
}

void DnsListener::close()
{
    qDeleteAll(m_udpSockets);
    m_udpSockets.clear();

    qDeleteAll(m_tcpServers);
    m_tcpServers.clear();
}

void DnsListener::sendResponse(const DnsClient &client, const QByteArray &response)
{
    QAbstractSocket *socket = client.socket;
    if (!socket || response.isEmpty())
        return;

    if (client.tcp) {
        writeTcpResponse(socket, response);
    } else {
        static_cast<QUdpSocket *>(socket)->writeDatagram(response, client.address, client.port);
    }
}

bool DnsListener::listenAddress(const QHostAddress &address)
{
    return listenUdp(address) && listenTcp(address);
}

bool DnsListener::listenUdp(const QHostAddress &address)
{
    auto socket = new QUdpSocket(this);

    connect(socket, &QUdpSocket::readyRead, this, [=, this] { readDatagrams(socket); });

    const bool ok = socket->bind(address, dnsPort, QAbstractSocket::DontShareAddress);

    return appendListener(m_udpSockets, socket, ok, address);
}

bool DnsListener::listenTcp(const QHostAddress &address)
{
    auto server = new QTcpServer(this);

    connect(server, &QTcpServer::newConnection, this, [=, this] { acceptConnections(server); });

    const bool ok = server->listen(address, dnsPort);

    return appendListener(m_tcpServers, server, ok, address);
}

void DnsListener::readDatagrams(QUdpSocket *socket)
{
    while (socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = socket->receiveDatagram();

        const DnsClient client = {
            .port = quint16(datagram.senderPort()),
            .address = datagram.senderAddress(),
            .socket = socket,
        };

        emit queryReceived(client, datagram.data());
    }
}

void DnsListener::acceptConnections(QTcpServer *server)
{
    while (QTcpSocket *socket = server->nextPendingConnection()) {
        connect(socket, &QTcpSocket::readyRead, this, [=, this] { readTcpQueries(socket); });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void DnsListener::readTcpQueries(QTcpSocket *socket)
{
    const DnsClient client = {
        .tcp = true,
        .port = socket->peerPort(),
        .address = socket->peerAddress(),
        .socket = socket,
    };

    QByteArray data;
    while (readTcpQuery(socket, data)) {
        emit queryReceived(client, data);
    }
}
