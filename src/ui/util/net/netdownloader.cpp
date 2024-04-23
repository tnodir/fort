#include "netdownloader.h"

#include <QLoggingCategory>
#include <QNetworkAccessManager>

#define DOWNLOAD_TIMEOUT (30 * 1000) // 30 seconds timeout
#define DOWNLOAD_MAXSIZE (16 * 1024 * 1024)

namespace {
const QLoggingCategory LC("util.net.downloader");
}

NetDownloader::NetDownloader(QObject *parent) :
    QObject(parent), m_manager(new QNetworkAccessManager(this))
{
    m_downloadTimer.setInterval(DOWNLOAD_TIMEOUT);

    connect(&m_downloadTimer, &QTimer::timeout, this, [&] {
        qCDebug(LC) << "Error: Download timed out";

        finish();
    });
}

QByteArray NetDownloader::takeBuffer()
{
    const QByteArray buf = m_buffer;
    m_buffer.clear();
    return buf;
}

void NetDownloader::start()
{
    qCDebug(LC) << "Fetch:" << url() << data();

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

    connect(m_reply, &QNetworkReply::downloadProgress, this, &NetDownloader::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &NetDownloader::onFinished);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &NetDownloader::onErrorOccurred);
    connect(m_reply, &QNetworkReply::sslErrors, this, &NetDownloader::onSslErrors);
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

void NetDownloader::onDownloadProgress(qint64 bytesReceived, qint64 /*bytesTotal*/)
{
    if (m_aborted || bytesReceived == 0)
        return;

    const QByteArray data = m_reply->read(DOWNLOAD_MAXSIZE - m_buffer.size());

    m_buffer.append(data);

    if (m_buffer.size() < DOWNLOAD_MAXSIZE) {
        emit progress(bytesReceived);
    } else {
        qCWarning(LC) << "Error: Too big file";
        finish();
    }
}

void NetDownloader::onFinished()
{
    const bool success = (m_reply->error() == QNetworkReply::NoError);

    finish(success && !m_buffer.isEmpty());
}

void NetDownloader::onErrorOccurred(QNetworkReply::NetworkError error)
{
    if (m_aborted)
        return;

    qCWarning(LC) << "Error:" << error << m_reply->errorString();

    finish();
}

void NetDownloader::onSslErrors(const QList<QSslError> &errors)
{
    if (m_aborted)
        return;

    for (const QSslError &error : errors) {
        qCWarning(LC) << "SSL Error:" << error.errorString();
    }

    finish();
}
