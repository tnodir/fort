#include "controlworker.h"

#include <QDataStream>
#include <QLocalSocket>

namespace {

constexpr int commandMaxArgs = 7;
constexpr int commandArgMaxSize = 4 * 1024;
constexpr int dataMaxSize = 4 * 1024 * 1024;

struct DataHeader
{
    DataHeader(Control::Command command = Control::CommandNone, int dataSize = 0) :
        m_command(command), m_dataSize(dataSize)
    {
    }

    Control::Command command() const { return static_cast<Control::Command>(m_command); }
    int dataSize() const { return m_dataSize; }

private:
    quint32 m_command : 8;
    quint32 m_dataSize : 24;
};

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
    DataHeader dataHeader(command, dataSize);
    socket()->write((const char *) &dataHeader, sizeof(DataHeader));
}

bool ControlWorker::readDataHeader(Control::Command &command, int &dataSize)
{
    DataHeader dataHeader;
    if (socket()->read((char *) &dataHeader, sizeof(DataHeader)) != sizeof(DataHeader))
        return false;

    command = dataHeader.command();
    dataSize = dataHeader.dataSize();

    if (dataSize > dataMaxSize)
        return false;

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

    const int argsCount = args.count();
    if (argsCount > commandMaxArgs)
        return false;

    stream << qint8(argsCount);

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

    qint8 argsCount;
    stream >> argsCount;
    if (argsCount > commandMaxArgs)
        return false;

    while (--argsCount >= 0) {
        QString arg;
        stream >> arg;
        if (arg.size() > commandArgMaxSize)
            return false;

        args.append(arg);
    }

    return true;
}
