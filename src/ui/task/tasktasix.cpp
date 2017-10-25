#include "tasktasix.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "../conf/addressgroup.h"
#include "../conf/firewallconf.h"
#ifndef TASK_TEST
#include "../fortmanager.h"
#endif
#include "../util/httpdownloader.h"
#include "../util/ip4range.h"

TaskTasix::TaskTasix(QObject *parent) :
    TaskWorker(parent),
    m_downloader(nullptr)
{
}

void TaskTasix::run()
{
    m_downloader = new HttpDownloader(this);

    connect(m_downloader, &HttpDownloader::finished,
            this, &TaskTasix::downloadFinished);

    m_rangeText = QString();

    startDownloader();
}

void TaskTasix::startDownloader() const
{
    downloader()->post(QUrl("http://mrlg.tas-ix.uz/index.cgi"),
                       "router=cisco&pass1=&query=1&arg=");
}

void TaskTasix::cancel(bool success)
{
    if (!m_downloader) return;

    m_downloader->cancel();
    m_downloader->deleteLater();
    m_downloader = nullptr;

    emit finished(success);
}

void TaskTasix::downloadFinished(bool success)
{
    if (success) {
        m_rangeText = parseBufer(m_downloader->buffer());
        success = !m_rangeText.isEmpty();
    }

    cancel(success);
}

bool TaskTasix::processResult(FortManager *fortManager)
{
#ifndef TASK_TEST
    FirewallConf *conf = fortManager->firewallConf();
    AddressGroup *ipExclude = conf->ipExclude();

    if (ipExclude->text() == m_rangeText)
        return false;

    ipExclude->setText(m_rangeText);

    return fortManager->saveOriginConf(tr("TAS-IX addresses updated!"));
#else
    Q_UNUSED(fortManager)

    return true;
#endif
}

QString TaskTasix::parseTasixBufer(const QByteArray &buffer)
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
