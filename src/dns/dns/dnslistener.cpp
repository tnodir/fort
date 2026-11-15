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
bool checkListening(
        QObject *listener, bool ok, const QHostAddress &address, const QString &errorString)
{
    if (ok)
        return true;

    qCWarning(LC) << "Listen error:" << listener->metaObject()->className() << address
                  << errorString;

    delete listener;

    return false;
}

}

DnsListener::DnsListener(QObject *parent) : QObject(parent) { }

bool DnsListener::listen()
{
    close();

    for (const auto address : { QHostAddress::LocalHost, QHostAddress::LocalHostIPv6 }) {
        listenUdp(address);
        listenTcp(address);
    }

    return isListening();
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

void DnsListener::listenUdp(const QHostAddress &address)
{
    auto socket = new QUdpSocket(this);

    const bool ok = socket->bind(address, dnsPort, QAbstractSocket::DontShareAddress);
    if (!checkListening(socket, ok, address, socket->errorString()))
        return;

    connect(socket, &QUdpSocket::readyRead, this, [=, this] { readDatagrams(socket); });

    m_udpSockets.append(socket);
}

void DnsListener::listenTcp(const QHostAddress &address)
{
    auto server = new QTcpServer(this);

    const bool ok = server->listen(address, dnsPort);
    if (!checkListening(server, ok, address, server->errorString()))
        return;

    connect(server, &QTcpServer::newConnection, this, [=, this] { acceptConnections(server); });

    m_tcpServers.append(server);
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
