#include "netdownloader.h"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define DOWNLOAD_TIMEOUT (30 * 1000) // 30 milliseconds timeout
#define DOWNLOAD_MAXSIZE (8 * 1024 * 1024)

NetDownloader::NetDownloader(QObject *parent) :
    QObject(parent), m_started(false), m_aborted(false), m_manager(new QNetworkAccessManager(this))
{
    m_downloadTimer.setInterval(DOWNLOAD_TIMEOUT);

    connect(&m_downloadTimer, &QTimer::timeout, this, [&] {
        qDebug() << "NetDownloader: Error: Download timed out";

        finish();
    });
}

void NetDownloader::start()
{
    qDebug() << "NetDownloader: Start:" << url() << data();

    m_started = true;
    m_aborted = false;

    m_buffer.clear();

    m_downloadTimer.start();

    QNetworkRequest request(url());

    if (!m_data.isEmpty()) {
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        m_reply = m_manager->post(request, data());
    } else {
        m_reply = m_manager->get(request);
    }

    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [&](qint64 bytesReceived, qint64 /*bytesTotal*/) {
                if (m_aborted || bytesReceived == 0)
                    return;

                const QByteArray data = m_reply->read(DOWNLOAD_MAXSIZE - m_buffer.size());

                m_buffer.append(data);
                if (m_buffer.size() >= DOWNLOAD_MAXSIZE) {
                    qWarning() << "NetDownloader: Error: Too big file";
                    finish();
                }
            });
    connect(m_reply, &QNetworkReply::finished, this, [&] {
        const bool success = (m_reply->error() == QNetworkReply::NoError);

        finish(success && !m_buffer.isEmpty());
    });
    connect(m_reply, &QNetworkReply::errorOccurred, this, [&](QNetworkReply::NetworkError error) {
        if (m_aborted)
            return;

        qWarning() << "NetDownloader: Error:" << error << m_reply->errorString();

        finish();
    });
    connect(m_reply, &QNetworkReply::sslErrors, this, [&](const QList<QSslError> &errors) {
        if (m_aborted)
            return;

        for (const QSslError &error : errors) {
            qWarning() << "NetDownloader: SSL Error:" << error.errorString();
        }

        finish();
    });
}

void NetDownloader::finish(bool success)
{
    if (!m_started || m_aborted)
        return;

    m_started = false;
    m_aborted = true;

    m_downloadTimer.stop();

    if (m_reply) {
        m_reply->abort();
        m_reply->close();

        m_reply->deleteLater();
        m_reply = nullptr;
    }

    emit finished(success);
}
