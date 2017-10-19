#include "tasktasix.h"

#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "../conf/addressgroup.h"
#include "../conf/firewallconf.h"
#ifndef TASK_TEST
#include "../fortmanager.h"
#endif
#include "../util/ip4range.h"

#define TASIX_MRLG_URL          "http://mrlg.tas-ix.uz/index.cgi"
#define TASIX_MRLG_DATA         "router=cisco&pass1=&query=1&arg="
#define TASIX_DOWNLOAD_TIMEOUT  30000  // 30 seconds timeout
#define TASIX_DOWNLOAD_MAXSIZE  (32 * 1024)

TaskTasix::TaskTasix(QObject *parent) :
    TaskWorker(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_reply(nullptr)
{
    connect(&m_timer, &QTimer::timeout, [this]() { cancel(); });

    m_timer.setSingleShot(true);
}

void TaskTasix::run()
{
    if (!QNetworkConfigurationManager().isOnline())
        return;

    QNetworkRequest request(QUrl(TASIX_MRLG_URL));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");

    m_reply = m_networkManager->post(request, TASIX_MRLG_DATA);

    connect(m_reply, &QIODevice::readyRead,
            this, &TaskTasix::requestReadyRead);
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &TaskTasix::requestError);
    connect(m_reply, &QNetworkReply::finished,
            this, &TaskTasix::requestFinished);

    m_rangeText = QString();

    m_timer.start(TASIX_DOWNLOAD_TIMEOUT);
}

void TaskTasix::cancel(bool success)
{
    if (!m_reply) return;

    m_reply->disconnect(this);  // to avoid recursive call on abort()

    m_reply->abort();
    m_reply->deleteLater();
    m_reply = nullptr;

    m_timer.stop();

    m_buffer.clear();

    emit finished(success);
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

void TaskTasix::requestError(int networkError)
{
    Q_UNUSED(networkError)

    cancel(false);
}

void TaskTasix::requestFinished()
{
    m_rangeText = parseBufer(m_buffer);

    cancel(!m_rangeText.isEmpty());
}

bool TaskTasix::processResult(FortManager *fortManager)
{
#ifndef TASK_TEST
    FirewallConf *conf = fortManager->firewallConf();
    AddressGroup *addressGroup = conf->ipExclude();

    if (addressGroup->text() == m_rangeText)
        return false;

    addressGroup->setText(m_rangeText);

    return fortManager->saveOriginConf(tr("TAS-IX addresses updated!"));
#else
    Q_UNUSED(fortManager)

    return true;
#endif
}

QString TaskTasix::parseBufer(const QByteArray &buffer)
{
    QStringList list;

    // Parse lines
    const QString text = QString::fromLatin1(buffer);
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

    if (list.isEmpty())
        return QString();

    // Include local networks
    list.append("10.0.0.0/8");
    list.append("127.0.0.0/8");
    list.append("169.254.0.0/16");
    list.append("172.16.0.0/12");
    list.append("192.168.0.0/16");

    // Merge lines
    Ip4Range ip4Range;
    if (!ip4Range.fromText(list.join('\n')))
        return QString();

    return ip4Range.toText();
}
