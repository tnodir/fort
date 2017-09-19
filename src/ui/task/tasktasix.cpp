#include "tasktasix.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "util/ip4range.h"

#define TASIX_MRLG_URL          "http://mrlg.tas-ix.uz/index.cgi"
#define TASIX_MRLG_DATA         "router=cisco&pass1=&query=1&arg="
#define TASIX_DOWNLOAD_TIMEOUT  30000  // 30 seconds timeout
#define TASIX_DOWNLOAD_MAXSIZE  (32 * 1024)

TaskTasix::TaskTasix(QObject *parent) :
    QObject(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_reply(nullptr)
{
    connect(&m_timer, &QTimer::timeout,
            this, &TaskTasix::cancel);

    m_timer.setSingleShot(true);
}

void TaskTasix::run()
{
    QNetworkRequest request(QUrl(TASIX_MRLG_URL));

    m_reply = m_networkManager->get(request);

    connect(m_reply, &QIODevice::readyRead,
            this, &TaskTasix::requestReadyRead);
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &TaskTasix::cancel);
    connect(m_reply, &QNetworkReply::finished,
            this, &TaskTasix::requestFinished);

    m_timer.start(TASIX_DOWNLOAD_TIMEOUT);
}

void TaskTasix::cancel()
{
    if (!m_reply) return;

    m_reply->abort();
    m_reply->deleteLater();
    m_reply = nullptr;

    m_buffer.clear();

    m_timer.stop();
}

void TaskTasix::requestReadyRead()
{
    const QByteArray data = m_reply->read(
                TASIX_DOWNLOAD_MAXSIZE - m_buffer.size());
    m_buffer.append(data);

    if (m_buffer.size() > TASIX_DOWNLOAD_MAXSIZE) {
        cancel();
    } else {
        m_timer.start();
    }
}

void TaskTasix::requestFinished()
{
    const QString text = parseBufer(m_buffer);

    cancel();

    emit finished();
}

QString TaskTasix::parseBufer(const QByteArray &buffer)
{
    QStringList list;

    // Parse lines
    const QString text(buffer);
    foreach (const QStringRef &line, text.splitRef('\n')) {
        if (!line.startsWith('*'))
            continue;

        QStringRef addrStr = line.mid(3, 18);
        const int lastSpacePos = addrStr.lastIndexOf(' ');
        if (lastSpacePos > 0) {
            addrStr = addrStr.left(lastSpacePos);
        }

        addrStr = addrStr.trimmed();
        if (addrStr.isEmpty())
            continue;

        list.append(addrStr.toString());
    }

    // Merge lines
    Ip4Range ip4Range;
    if (!ip4Range.fromText(list.join('\n')))
        return QString();

    return ip4Range.toText();
}
