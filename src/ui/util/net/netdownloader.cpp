#include "netdownloader.h"

#define DOWNLOAD_TIMEOUT  30  // 30 seconds timeout
#define DOWNLOAD_MAXSIZE  (64 * 1024)

NetDownloader::NetDownloader(QObject *parent) :
    QObject(parent),
    m_started(false)
{
    setupProcess();
}

void NetDownloader::setupProcess()
{
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &NetDownloader::processReadyRead);
    connect(&m_process, &QProcess::errorOccurred,
            this, &NetDownloader::processError);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NetDownloader::processFinished);
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
    m_buffer.clear();

    m_process.start("curl", args, QIODevice::ReadOnly);

    if (!m_process.waitForStarted()) {
        abort();
    }
}

void NetDownloader::abort(bool success)
{
    if (!m_started) return;

    m_started = false;

    m_process.kill();
    m_process.waitForFinished();

    emit finished(success);
}

void NetDownloader::processReadyRead()
{
    const QByteArray data = m_process.read(
                DOWNLOAD_MAXSIZE - m_buffer.size());
    m_buffer.append(data);

    if (m_buffer.size() > DOWNLOAD_MAXSIZE) {
        abort(true);  // try to use the partial loaded data
    }
}

void NetDownloader::processError(QProcess::ProcessError error)
{
    Q_UNUSED(error)

    abort();
}

void NetDownloader::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    abort(exitCode == 0 && exitStatus == QProcess::NormalExit
          && !m_buffer.isEmpty());
}
