#ifndef NETDOWNLOADER_H
#define NETDOWNLOADER_H

#include <QProcess>

class NetDownloader : public QObject
{
    Q_OBJECT

public:
    explicit NetDownloader(QObject *parent = nullptr);

    void setUrl(const QString &url) { m_url = url; }
    void setData(const QByteArray &data) { m_data = data; }

    QByteArray buffer() const { return m_buffer; }

signals:
    void finished(bool success);

public slots:
    void start();
    void abort(bool success = false);

private slots:
    void processReadyRead();
    void processError(QProcess::ProcessError error);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupProcess();

private:
    bool m_started;

    QString m_url;
    QByteArray m_data;

    QProcess m_process;

    QByteArray m_buffer;
};

#endif // NETDOWNLOADER_H
