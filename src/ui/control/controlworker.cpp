#include "controlworker.h"

#include <QDataStream>
#include <QLocalSocket>
#include <QLoggingCategory>

namespace {

const QLoggingCategory LC("control");

constexpr int commandMaxArgs = 16;
constexpr int commandArgMaxSize = 4 * 1024;
constexpr quint32 dataMaxSize = 1 * 1024 * 1024;

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
        QDataStream stream(&data,
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                QIODevice::WriteOnly
#else
                QDataStream::WriteOnly
#endif
        );

        stream << qint8(argsCount);

        for (const auto &arg : args) {
            stream << arg;
        }
    }

    compressed = (data.size() > 128);
    buffer = compressed ? qCompress(data) : data;

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
    QObject(parent),
    m_isServiceClient(false),
    m_isClientValidated(false),
    m_isTryReconnect(false),
    m_socket(socket)
{
}

int ControlWorker::id() const
{
    return int(socket()->socketDescriptor());
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
                qCWarning(LC) << "Client error:" << id() << socketError << errorString();
                close();
            });
    connect(socket(), &QLocalSocket::disconnected, this, &ControlWorker::onDisconnected);
    connect(socket(), &QLocalSocket::readyRead, this, &ControlWorker::processRequest);
}

void ControlWorker::setServerName(const QString &name)
{
    socket()->setServerName(name);
}

bool ControlWorker::connectToServer()
{
    socket()->connectToServer();

    if (!socket()->waitForConnected(150)) {
        qCWarning(LC) << "Connection error:" << socket()->state() << socket()->errorString();
        return false;
    }

    if (socket()->state() != QLocalSocket::ConnectedState) {
        qCWarning(LC) << "Connection state error:" << socket()->state();
        return false;
    }

    return true;
}

void ControlWorker::close()
{
    socket()->abort();
    socket()->close();
}

void ControlWorker::onDisconnected()
{
    if (isTryReconnect() && connectToServer())
        return;

    emit disconnected();
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
    const QByteArray buffer = buildCommandData(command, args);
    if (buffer.isEmpty()) {
        qCWarning(LC) << "Bad RPC command to send:" << command << args;
        return false;
    }

    return sendCommandData(buffer);
}

bool ControlWorker::waitForSent(int msecs) const
{
    return socket()->waitForBytesWritten(msecs);
}

bool ControlWorker::waitForRead(int msecs) const
{
    return socket()->waitForReadyRead(msecs);
}

void ControlWorker::processRequest()
{
    while (socket()->bytesAvailable() >= sizeof(RequestHeader)) {
        if (!readRequest()) {
            close();
            clearRequest();
            break;
        }
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

    // qCDebug(LC) << "requestReady>" << id() << command << args;

    emit requestReady(command, args);

    return true;
}

bool ControlWorker::readRequestHeader()
{
    const int headerSize = socket()->read((char *) &m_requestHeader, sizeof(RequestHeader));
    if (headerSize != sizeof(RequestHeader)) {
        qCWarning(LC) << "Bad request header:"
                      << "size=" << headerSize;
        return false;
    }

    if (m_requestHeader.command() == Control::CommandNone
            || m_requestHeader.dataSize() > dataMaxSize) {
        qCWarning(LC) << "Bad request:"
                      << "command=" << m_requestHeader.command()
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
