#include "httpdownloader.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#define DOWNLOAD_TIMEOUT  30000  // 30 seconds timeout
#define DOWNLOAD_MAXSIZE  (64 * 1024)

HttpDownloader::HttpDownloader(QObject *parent) :
    QObject(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_reply(nullptr)
{
    connect(&m_timer, &QTimer::timeout, [this]() { cancel(); });

    m_timer.setSingleShot(true);
}

void HttpDownloader::get(const QUrl &url)
{
    QNetworkRequest request(url);

    m_reply = m_networkManager->get(request);

    prepareReply();
}

void HttpDownloader::post(const QUrl &url, const QByteArray &data)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    m_reply = m_networkManager->post(request, data);

    prepareReply();
}

void HttpDownloader::prepareReply()
{
    Q_ASSERT(m_reply);

    connect(m_reply, &QIODevice::readyRead,
            this, &HttpDownloader::requestReadyRead);
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &HttpDownloader::requestError);
    connect(m_reply, &QNetworkReply::finished,
            this, &HttpDownloader::requestFinished);

    m_timer.start(DOWNLOAD_TIMEOUT);

    m_buffer.clear();
}

void HttpDownloader::cancel(bool success)
{
    if (!m_reply) return;

    m_reply->disconnect(this);  // to avoid recursive call on abort()

    m_reply->abort();
    m_reply->deleteLater();
    m_reply = nullptr;

    m_timer.stop();

    emit finished(success);
}

void HttpDownloader::requestReadyRead()
{
    const QByteArray data = m_reply->read(
                DOWNLOAD_MAXSIZE - m_buffer.size());
    m_buffer.append(data);

    if (m_buffer.size() > DOWNLOAD_MAXSIZE) {
        cancel(true);  // try to use the partial loaded data
    } else {
        m_timer.start();
    }
}

void HttpDownloader::requestError(int networkError)
{
    Q_UNUSED(networkError)

    cancel();
}

void HttpDownloader::requestFinished()
{
    cancel(!m_buffer.isEmpty());
}
