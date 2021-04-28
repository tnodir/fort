#ifndef CONTROLWORKER_H
#define CONTROLWORKER_H

#include <QObject>
#include <QVariant>

#include "control.h"

QT_FORWARD_DECLARE_CLASS(QLocalSocket)

class ControlWorker : public QObject
{
    Q_OBJECT

public:
    explicit ControlWorker(QLocalSocket *socket, QObject *parent = nullptr);

    bool isServiceClient() const { return m_isServiceClient; }
    void setIsServiceClient(bool v) { m_isServiceClient = v; }

    QLocalSocket *socket() const { return m_socket; }

    void setupForAsync();

    bool sendCommand(Control::Command command, Control::RpcObject rpcObj, int methodIndex,
            const QVariantList &args);

    bool waitForSent(int msecs = 1000) const;

    static QVariantList buildArgs(const QStringList &list);

signals:
    void requestReady(Control::Command command, const QVariantList &args);

public slots:
    void abort();

private slots:
    void processRequest();

private:
    void clearRequest();
    bool readRequest();

    bool readRequestHeader();

private:
    struct DataHeader
    {
        DataHeader(Control::Command command = Control::CommandNone,
                Control::RpcObject rpcObj = Control::Rpc_None, qint16 methodIndex = 0,
                qint32 dataSize = 0) :
            m_command(command), m_rpcObj(rpcObj), m_methodIndex(methodIndex), m_dataSize(dataSize)
        {
        }

        Control::Command command() const { return m_command; }
        Control::RpcObject rpcObj() const { return m_rpcObj; }
        qint16 methodIndex() const { return m_methodIndex; }
        qint32 dataSize() const { return m_dataSize; }

        void clear()
        {
            m_command = Control::CommandNone;
            m_rpcObj = Control::Rpc_None;
            m_methodIndex = 0;
            m_dataSize = 0;
        }

    private:
        Control::Command m_command;
        Control::RpcObject m_rpcObj;
        qint16 m_methodIndex;
        quint32 m_dataSize;
    };

    bool m_isServiceClient = false;

    DataHeader m_request;
    QByteArray m_requestData;

    QLocalSocket *m_socket = nullptr;
};

#endif // CONTROLWORKER_H
