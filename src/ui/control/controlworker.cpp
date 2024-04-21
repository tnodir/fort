#include "controlworker.h"

#include <QDataStream>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QTimer>

namespace {

const QLoggingCategory LC("controlWorker");

constexpr int commandMaxArgs = 32;
constexpr int commandArgMaxSize = 4 * 1024;
constexpr quint32 dataMaxSize = 1 * 1024 * 1024;

quint32 nextWorkerId()
{
    static quint32 g_workerId = 0;
    return ++g_workerId;
}

bool buildArgsData(QByteArray &buffer, const QVariantList &args, bool &compressed)
{
    const int argsCount = args.count();
    if (argsCount == 0)
        return true;

    if (argsCount > commandMaxArgs) {
        qCWarning(LC) << "Bad build args count:" << argsCount;
        return false;
    }

    QByteArray data;
    {
        QDataStream stream(&data, QDataStream::WriteOnly);

        stream << qint8(argsCount);

        for (const auto &arg : args) {
            stream << arg;
        }
    }

    compressed = (data.size() > 128);
    buffer = compressed ? qCompress(data) : std::move(data);

    return true;
}

bool parseArgsData(const QByteArray &buffer, QVariantList &args, bool compressed)
{
    if (buffer.isEmpty())
        return true;

    const QByteArray data = compressed ? qUncompress(buffer) : buffer;

    QDataStream stream(data);

    qint8 argsCount;
    stream >> argsCount;

    if (argsCount > commandMaxArgs) {
        qCWarning(LC) << "Bad parse args count:" << argsCount;
        return false;
    }

    while (--argsCount >= 0) {
        QVariant arg;
        stream >> arg;

        args.append(arg);
    }

    return true;
}

}

ControlWorker::ControlWorker(QLocalSocket *socket, QObject *parent) :
    QObject(parent), m_id(nextWorkerId()), m_socket(socket)
{
}

bool ControlWorker::isConnected() const
{
    return socket()->state() == QLocalSocket::ConnectedState;
}

QString ControlWorker::errorString() const
{
    return socket()->errorString();
}

void ControlWorker::setupForAsync()
{
    socket()->setParent(this);

    connect(socket(), &QLocalSocket::errorOccurred, this,
            [&](QLocalSocket::LocalSocketError socketError) {
                if (!m_isReconnecting) {
                    qCWarning(LC) << "Client error:" << id() << socketError << errorString();
                }
                close();
            });
    connect(socket(), &QLocalSocket::connected, this, &ControlWorker::onConnected);
    connect(socket(), &QLocalSocket::disconnected, this, &ControlWorker::onDisconnected);
    connect(socket(), &QLocalSocket::readyRead, this, &ControlWorker::processRequest);
}

void ControlWorker::setServerName(const QString &v)
{
    m_serverName = v;
}

bool ControlWorker::connectToServer()
{
    socket()->connectToServer(serverName());

    if (!socket()->waitForConnected(100)) {
        if (!m_isReconnecting) {
            qCWarning(LC) << "Connection error:" << socket()->state() << socket()->errorString();
        }
        return false;
    }

    return true;
}

bool ControlWorker::reconnectToServer()
{
    if (m_isReconnecting)
        return false;

    m_isReconnecting = true;

    close();

    bool connectedToServer;

    int reconnectCount = 3;
    do {
        connectedToServer = connectToServer();
        if (connectedToServer)
            break;
    } while (--reconnectCount > 0);

    m_isReconnecting = false;

    if (!connectedToServer) {
        startReconnectTimer();
    }

    return connectedToServer;
}

void ControlWorker::close()
{
    socket()->abort();
    socket()->close();
}

void ControlWorker::onConnected()
{
    stopReconnectTimer();

    emit connected();
}

void ControlWorker::onDisconnected()
{
    if (m_processing > 0)
        return;

    if (isTryReconnect() && reconnectToServer())
        return;

    emit disconnected();

    startReconnectTimer();
}

void ControlWorker::startReconnectTimer()
{
    if (!isTryReconnect())
        return;

    if (!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setInterval(1000);

        connect(m_reconnectTimer, &QTimer::timeout, this, &ControlWorker::reconnectToServer);
    }

    m_reconnectTimer->start();
}

void ControlWorker::stopReconnectTimer()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
}

QByteArray ControlWorker::buildCommandData(Control::Command command, const QVariantList &args)
{
    QByteArray data;
    bool compressed = false;
    if (!buildArgsData(data, args, compressed))
        return {};

    RequestHeader request(command, compressed, data.size());

    QByteArray buffer;
    buffer.append((const char *) &request, sizeof(RequestHeader));
    buffer.append(data);

    return buffer;
}

bool ControlWorker::sendCommandData(const QByteArray &commandData)
{
    Q_ASSERT(!commandData.isEmpty());

    const int bytesSent = socket()->write(commandData);
    if (bytesSent != commandData.size()) {
        if (bytesSent < 0) {
            qCWarning(LC) << "Send error:" << id() << errorString();
        } else {
            qCWarning(LC) << "Sent partial:" << id() << bytesSent << commandData.size();
        }
        return false;
    }

    socket()->flush();

    return true;
}

bool ControlWorker::sendCommand(Control::Command command, const QVariantList &args)
{
    // DBG: qCDebug(LC) << "Send Command: id:" << id() << command << args.size();

    const QByteArray buffer = buildCommandData(command, args);
    if (buffer.isEmpty()) {
        qCWarning(LC) << "Bad RPC command to send:" << command << args;
        return false;
    }

    return sendCommandData(buffer);
}

bool ControlWorker::waitForSent(int msecs) const
{
    return socket()->bytesToWrite() <= 0 || socket()->waitForBytesWritten(msecs);
}

bool ControlWorker::waitForRead(int msecs) const
{
    return socket()->waitForReadyRead(msecs);
}

void ControlWorker::processRequest()
{
    ++m_processing;

    while (socket()->bytesAvailable() >= int(sizeof(RequestHeader))) {
        if (!readRequest()) {
            close();
            clearRequest();
            break;
        }
    }

    --m_processing;

    if (!isConnected()) {
        onDisconnected();
    }
}

void ControlWorker::clearRequest()
{
    m_requestHeader.clear();
    m_requestBuffer.clear();
}

bool ControlWorker::readRequest()
{
    if (m_requestHeader.command() == Control::CommandNone && !readRequestHeader())
        return false;

    const int bytesNeeded = m_requestHeader.dataSize() - m_requestBuffer.size();
    if (bytesNeeded > 0) {
        if (socket()->bytesAvailable() == 0)
            return true; // need more data

        const QByteArray data = socket()->read(bytesNeeded);
        if (data.isEmpty()) {
            qCWarning(LC) << "Bad request: empty";
            return false;
        }

        m_requestBuffer += data;

        if (data.size() < bytesNeeded)
            return true; // need more data
    }

    QVariantList args;
    if (!parseArgsData(m_requestBuffer, args, m_requestHeader.compressed()))
        return false;

    const Control::Command command = m_requestHeader.command();

    clearRequest();

    // DBG: qCDebug(LC) << "requestReady>" << id() << command << args;

    emit requestReady(command, args);

    return true;
}

bool ControlWorker::readRequestHeader()
{
    const int headerSize = socket()->read((char *) &m_requestHeader, sizeof(RequestHeader));
    if (headerSize != sizeof(RequestHeader)) {
        qCWarning(LC) << "Bad request header:" << "size=" << headerSize;
        return false;
    }

    if (m_requestHeader.command() == Control::CommandNone
            || m_requestHeader.dataSize() > dataMaxSize) {
        qCWarning(LC) << "Bad request:" << "command=" << m_requestHeader.command()
                      << "size=" << m_requestHeader.dataSize();
        return false;
    }

    return true;
}

QVariantList ControlWorker::buildArgs(const QStringList &list)
{
    QVariantList args;

    for (const auto &s : list) {
        if (s.size() > commandArgMaxSize)
            return {};

        args.append(s);
    }

    return args;
}
