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

void NetDownloader::setStarted(bool v)
{
    if (m_started != v) {
        m_started = v;
        emit startedChanged(v);
    }
}

QByteArray NetDownloader::takeBuffer()
{
    QByteArray buf;
    m_buffer.swap(buf);
    return buf;
}

void NetDownloader::start()
{
    qCDebug(LC) << "Fetch:" << url() << data();

    setStarted(true);
    setAborted(false);

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
    if (!started() || aborted())
        return;

    setStarted(false);
    setAborted(true);

    m_downloadTimer.stop();

    if (m_reply) {
        m_reply->abort();
        m_reply->close();

        m_reply->deleteLater();
        m_reply = nullptr;
    }

    emit finished(takeBuffer(), success);
}

void NetDownloader::onDownloadProgress(qint64 bytesReceived, qint64 /*bytesTotal*/)
{
    if (m_aborted || bytesReceived == 0)
        return;

    const QByteArray data = m_reply->read(DOWNLOAD_MAXSIZE - m_buffer.size());

    m_buffer.append(data);

    const int bufferSize = m_buffer.size();
    if (bufferSize < DOWNLOAD_MAXSIZE) {
        emit dataReceived(bufferSize);
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
