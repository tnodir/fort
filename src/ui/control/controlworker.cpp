#include "controlworker.h"

#include <QDataStream>
#include <QLocalSocket>

namespace {

constexpr int commandMaxArgs = 7;
constexpr int commandArgMaxSize = 256;

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

bool ControlWorker::postCommand(Control::Command command, const QStringList &args)
{
    QByteArray data;
    if (!buildArgsData(data, args))
        return false;

    writeDataHeader(command, data.size());
    writeData(data);

    return socket()->waitForBytesWritten(1000);
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
    m_requestCommand = Control::CommandNone;
    m_requestDataSize = 0;
    m_requestData.clear();
}

bool ControlWorker::readRequest()
{
    if (m_requestCommand == Control::CommandNone
            && !readDataHeader(m_requestCommand, m_requestDataSize))
        return false;

    if (m_requestDataSize > 0) {
        if (socket()->bytesAvailable() == 0)
            return true; // need more data

        const QByteArray data = readData(m_requestDataSize);
        if (data.isEmpty())
            return false;

        m_requestData += data;
        m_requestDataSize -= data.size();

        if (m_requestDataSize > 0)
            return true; // need more data
    }

    QStringList args;
    if (!m_requestData.isEmpty() && !parseArgsData(m_requestData, args))
        return false;

    emit requestReady(m_requestCommand, args);
    clearRequest();

    return true;
}

void ControlWorker::writeDataHeader(Control::Command command, int dataSize)
{
    const quint32 dataHeader = (quint32(command) << 24) | dataSize;
    socket()->write((const char *) &dataHeader, sizeof(quint32));
}

bool ControlWorker::readDataHeader(Control::Command &command, int &dataSize)
{
    quint32 dataHeader = 0;
    if (socket()->read((char *) &dataHeader, sizeof(quint32)) != sizeof(quint32))
        return false;

    command = static_cast<Control::Command>(dataHeader >> 24);
    dataSize = dataHeader & 0xFFFFFF;

    return true;
}

void ControlWorker::writeData(const QByteArray &data)
{
    socket()->write(data);
}

QByteArray ControlWorker::readData(int dataSize)
{
    return socket()->read(dataSize);
}

bool ControlWorker::buildArgsData(QByteArray &data, const QStringList &args)
{
    QDataStream stream(&data,
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QIODevice::WriteOnly
#else
            QDataStream::WriteOnly
#endif
    );

    const int argsSize = args.size();
    if (argsSize > commandMaxArgs)
        return false;

    stream << qint8(argsSize);

    for (const auto &arg : args) {
        if (arg.size() > commandArgMaxSize)
            return false;

        stream << arg;
    }

    return true;
}

bool ControlWorker::parseArgsData(const QByteArray &data, QStringList &args)
{
    QDataStream stream(data);

    qint8 argsSize;
    stream >> argsSize;
    if (argsSize > commandMaxArgs)
        return false;

    while (--argsSize >= 0) {
        QString arg;
        stream >> arg;
        if (arg.size() > commandArgMaxSize)
            return false;

        args.append(arg);
    }

    return true;
}
