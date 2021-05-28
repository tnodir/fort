#include "netdownloader.h"

#include <QDebug>

#define DOWNLOAD_TIMEOUT 30 // 30 seconds timeout
#define DOWNLOAD_MAXSIZE (64 * 1024)

NetDownloader::NetDownloader(QObject *parent) : QObject(parent), m_started(false), m_aborted(false)
{
    setupProcess();
}

void NetDownloader::setupProcess()
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &NetDownloader::processReadyRead);
    connect(&m_process, &QProcess::errorOccurred, this, &NetDownloader::processError);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &NetDownloader::processFinished);
}

void NetDownloader::start()
{
    QStringList args;

    args.append("--max-time");
    args.append(QString::number(DOWNLOAD_TIMEOUT));

    if (!m_data.isEmpty()) {
        args.append("--data");
        args.append(QString::fromLatin1(m_data.constData()));
    }

    args.append(m_url);

    m_started = true;
    m_aborted = false;

    m_buffer.clear();

    m_process.start("curl", args, QIODevice::ReadOnly);

    qDebug() << "NetDownloader: Run `curl`:" << args;

    if (!m_process.waitForStarted(1000)) {
        qWarning() << "NetDownloader: Cannot start `curl`:" << m_process.errorString();

        abort();
    }
}

void NetDownloader::abort(bool success)
{
    if (!m_started || m_aborted)
        return;

    m_started = false;
    m_aborted = true;

    m_process.kill();
    m_process.waitForFinished();

    emit finished(success);
}

void NetDownloader::processReadyRead()
{
    const QByteArray data = m_process.read(DOWNLOAD_MAXSIZE - m_buffer.size());
    m_buffer.append(data);

    if (m_buffer.size() > DOWNLOAD_MAXSIZE) {
        abort(true); // try to use the partial loaded data
    }
}

void NetDownloader::processError(QProcess::ProcessError error)
{
    if (m_aborted)
        return;

    qWarning() << "NetDownloader: Cannot run `curl`:" << error << m_process.errorString();

    abort();
}

void NetDownloader::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    if (!success) {
        qWarning() << "NetDownloader: `curl` error code:" << exitCode;
    }

    abort(success && !m_buffer.isEmpty());
}
