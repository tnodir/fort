#include "controlworker.h"

#include <QDataStream>
#include <QLocalSocket>

namespace {

constexpr int commandMaxArgs = 7;
constexpr int commandArgMaxSize = 4 * 1024;
constexpr int dataMaxSize = 4 * 1024 * 1024;

template<typename T>
T *bufferAs(QByteArray &buffer, int offset = 0)
{
    return reinterpret_cast<T *>(buffer.data() + offset);
}

bool buildArgsData(QByteArray &data, const QVariantList &args)
{
    const int argsCount = args.count();
    if (argsCount == 0)
        return true;

    if (argsCount > commandMaxArgs)
        return false;

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

    return true;
}

bool parseArgsData(const QByteArray &data, QVariantList &args)
{
    QDataStream stream(data);

    qint8 argsCount;
    stream >> argsCount;

    if (argsCount > commandMaxArgs)
        return false;

    while (--argsCount >= 0) {
        QVariant arg;
        stream >> arg;

        args.append(arg);
    }

    return true;
}

}

ControlWorker::ControlWorker(QLocalSocket *socket, QObject *parent) :
    QObject(parent), m_socket(socket)
{
}

void ControlWorker::setupForAsync()
{
    socket()->setParent(this);

    connect(socket(), &QLocalSocket::disconnected, this, &QObject::deleteLater);
    connect(socket(), &QLocalSocket::readyRead, this, &ControlWorker::processRequest);
}

void ControlWorker::abort()
{
    socket()->abort();
    socket()->close();
}

bool ControlWorker::sendCommand(Control::Command command, Control::RpcObject rpcObj,
        int methodIndex, const QVariantList &args)
{
    QByteArray buffer;

    if (!buildArgsData(buffer, args))
        return false;

    new (bufferAs<DataHeader>(buffer))
            DataHeader(command, rpcObj, methodIndex, buffer.size() - sizeof(DataHeader));

    socket()->write(buffer);

    return true;
}

bool ControlWorker::waitForSent(int msecs) const
{
    return socket()->waitForBytesWritten(msecs);
}

void ControlWorker::processRequest()
{
    if (!readRequest()) {
        abort();
        clearRequest();
    }
}

void ControlWorker::clearRequest()
{
    m_request.clear();
    m_requestData.clear();
}

bool ControlWorker::readRequest()
{
    if (m_request.command() == Control::CommandNone && !readRequestHeader())
        return false;

    if (m_request.dataSize() > 0) {
        if (socket()->bytesAvailable() == 0)
            return true; // need more data

        const QByteArray data = socket()->read(m_request.dataSize() - m_requestData.size());
        if (data.isEmpty())
            return false;

        m_requestData += data;

        if (m_requestData.size() < m_request.dataSize())
            return true; // need more data
    }

    QVariantList args;
    if (!m_requestData.isEmpty() && !parseArgsData(m_requestData, args))
        return false;

    emit requestReady(m_request.command(), args);
    clearRequest();

    return true;
}

bool ControlWorker::readRequestHeader()
{
    if (socket()->read((char *) &m_request, sizeof(DataHeader)) != sizeof(DataHeader))
        return false;

    if (m_request.dataSize() > dataMaxSize)
        return false;

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
