#ifndef CONTROLWORKER_H
#define CONTROLWORKER_H

#include <QObject>

#include "control.h"

QT_FORWARD_DECLARE_CLASS(QLocalSocket)

class ControlWorker : public QObject
{
    Q_OBJECT

public:
    explicit ControlWorker(QLocalSocket *socket, QObject *parent = nullptr);

    QLocalSocket *socket() const { return m_socket; }

    void setupForAsync();

    bool postCommand(Control::Command command, const QStringList &args);

signals:
    void requestReady(Control::Command command, const QStringList &args);

public slots:
    void abort();

private slots:
    void processRequest();

private:
    void clearRequest();
    bool readRequest();

    void writeDataHeader(Control::Command command, int dataSize);
    bool readDataHeader(Control::Command &command, int &dataSize);

    void writeData(const QByteArray &data);
    QByteArray readData(int dataSize);

    static bool buildArgsData(QByteArray &data, const QStringList &args);
    static bool parseArgsData(const QByteArray &data, QStringList &args);

private:
    Control::Command m_requestCommand = Control::CommandNone;
    int m_requestDataSize = 0;
    QByteArray m_requestData;

    QLocalSocket *m_socket = nullptr;
};

#endif // CONTROLWORKER_H
