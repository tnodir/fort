#ifndef CONTROLWORKER_H
#define CONTROLWORKER_H

#include <QObject>
#include <QVariant>

#include "control.h"

QT_FORWARD_DECLARE_CLASS(QLocalSocket)
QT_FORWARD_DECLARE_CLASS(QTimer)

class ControlWorker : public QObject
{
    Q_OBJECT

public:
    explicit ControlWorker(QLocalSocket *socket, QObject *parent = nullptr);

    bool isServiceClient() const { return m_isServiceClient; }
    void setIsServiceClient(bool v) { m_isServiceClient = v; }

    bool isClientValidated() const { return m_isClientValidated; }
    void setIsClientValidated(bool v) { m_isClientValidated = v; }

    bool isTryReconnect() const { return m_isTryReconnect; }
    void setIsTryReconnect(bool v) { m_isTryReconnect = v; }

    quint32 id() const { return m_id; }

    QString serverName() const { return m_serverName; }
    void setServerName(const QString &v);

    QLocalSocket *socket() const { return m_socket; }

    bool isConnected() const;
    QString errorString() const;

    void setupForAsync();

    bool connectToServer();
    bool reconnectToServer();

    static QByteArray buildCommandData(Control::Command command, const QVariantList &args = {});
    bool sendCommandData(const QByteArray &commandData);

    bool sendCommand(Control::Command command, const QVariantList &args = {});

    bool waitForSent(int msecs = 700) const;
    bool waitForRead(int msecs = 700) const;

    static QVariantList buildArgs(const QStringList &list);

signals:
    void connected();
    void disconnected();
    void requestReady(Control::Command command, const QVariantList &args);

public slots:
    void close();

private slots:
    void onConnected();
    void onDisconnected();

    void startReconnectTimer();
    void stopReconnectTimer();

    void processRequest();

private:
    void clearRequest();
    bool readRequest();

    bool readRequestHeader();

protected:
    struct RequestHeader
    {
        RequestHeader(Control::Command command = Control::CommandNone, bool compressed = false,
                quint32 dataSize = 0) :
            m_command(command), m_compressed(compressed), m_dataSize(dataSize)
        {
        }

        Control::Command command() const { return static_cast<Control::Command>(m_command); }
        bool compressed() const { return m_compressed; }
        quint32 dataSize() const { return m_dataSize; }

        void clear()
        {
            m_command = Control::CommandNone;
            m_compressed = false;
            m_dataSize = 0;
        }

    private:
        quint32 m_command : 7;
        quint32 m_compressed : 1;
        quint32 m_dataSize : 24;
    };

private:
    bool m_isServiceClient : 1 = false;
    bool m_isClientValidated : 1 = false;
    bool m_isTryReconnect : 1 = false;
    bool m_isReconnecting : 1 = false;

    QAtomicInt m_processing = 0;

    const quint32 m_id = 0;

    RequestHeader m_requestHeader;
    QByteArray m_requestBuffer;

    QString m_serverName;
    QLocalSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
};

#endif // CONTROLWORKER_H
