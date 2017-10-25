#ifndef HTTPDOWNLOADER_H
#define HTTPDOWNLOADER_H

#include <QObject>
#include <QTimer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class HttpDownloader : public QObject
{
    Q_OBJECT

public:
    explicit HttpDownloader(QObject *parent = nullptr);

    QByteArray buffer() const { return m_buffer; }

signals:
    void finished(bool success);

public slots:
    void get(const QUrl &url);
    void post(const QUrl &url, const QByteArray &data);

    void cancel(bool success = false);

private slots:
    void requestReadyRead();
    void requestError(int networkError);
    void requestFinished();

private:
    void prepareReply();

private:
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_reply;

    QTimer m_timer;

    QByteArray m_buffer;
};

#endif // HTTPDOWNLOADER_H
