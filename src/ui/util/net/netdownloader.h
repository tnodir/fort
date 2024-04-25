#ifndef NETDOWNLOADER_H
#define NETDOWNLOADER_H

#include <QNetworkReply>
#include <QObject>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)

class NetDownloader : public QObject
{
    Q_OBJECT

public:
    explicit NetDownloader(QObject *parent = nullptr);

    bool started() const { return m_started; }
    void setStarted(bool v);

    bool aborted() const { return m_aborted; }
    void setAborted(bool v) { m_aborted = v; }

    QString url() const { return m_url; }
    void setUrl(const QString &url) { m_url = url; }

    QByteArray data() const { return m_data; }
    void setData(const QByteArray &data) { m_data = data; }

    QByteArray buffer() const { return m_buffer; }
    void setBuffer(const QByteArray &buffer) { m_buffer = buffer; }

    QByteArray takeBuffer();

signals:
    void startedChanged(bool started);
    void dataReceived(int size);
    void finished(const QByteArray &data, bool success);

public slots:
    void start();
    void finish(bool success = false);

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onFinished();
    void onErrorOccurred(QNetworkReply::NetworkError error);
    void onSslErrors(const QList<QSslError> &errors);

private:
    bool m_started : 1 = false;
    bool m_aborted : 1 = false;

    QString m_url;
    QByteArray m_data;

    QByteArray m_buffer;

    QTimer m_downloadTimer;

    QNetworkAccessManager *m_manager = nullptr;
    QNetworkReply *m_reply = nullptr;
};

#endif // NETDOWNLOADER_H
