#ifndef NETDOWNLOADER_H
#define NETDOWNLOADER_H

#include <QObject>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)

class NetDownloader : public QObject
{
    Q_OBJECT

public:
    explicit NetDownloader(QObject *parent = nullptr);

    QString url() const { return m_url; }
    void setUrl(const QString &url) { m_url = url; }

    QByteArray data() const { return m_data; }
    void setData(const QByteArray &data) { m_data = data; }

    QByteArray buffer() const { return m_buffer; }
    void setBuffer(const QByteArray &buffer) { m_buffer = buffer; }

signals:
    void finished(bool success);

public slots:
    void start();
    void finish(bool success = false);

private:
    bool m_started : 1;
    bool m_aborted : 1;

    QString m_url;
    QByteArray m_data;

    QByteArray m_buffer;

    QTimer m_downloadTimer;

    QNetworkAccessManager *m_manager = nullptr;
    QNetworkReply *m_reply = nullptr;
};

#endif // NETDOWNLOADER_H
