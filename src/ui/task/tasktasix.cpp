#include "tasktasix.h"

#include "../conf/addressgroup.h"
#include "../conf/firewallconf.h"
#ifndef TASK_TEST
#include "../fortmanager.h"
#endif
#include "../util/ip4range.h"
#include "../util/netdownloader.h"

TaskTasix::TaskTasix(QObject *parent) :
    TaskWorker(parent),
    m_downloader(nullptr)
{
}

void TaskTasix::run()
{
    m_downloader = new NetDownloader(this);

    connect(m_downloader, &NetDownloader::finished,
            this, &TaskTasix::downloadFinished);

    m_rangeText = QString();

    setupDownloader();

    downloader()->start();
}

void TaskTasix::setupDownloader() const
{
    downloader()->setUrl("http://mrlg.tas-ix.uz/index.cgi");
    downloader()->setData("router=cisco&pass1=&query=1&arg=");
}

void TaskTasix::cancel(bool success)
{
    if (!m_downloader) return;

    m_downloader->disconnect(this);  // to avoid recursive call on cancel()

    m_downloader->cancel();
    m_downloader->deleteLater();
    m_downloader = nullptr;

    emit finished(success);
}

void TaskTasix::downloadFinished(bool success)
{
    if (success) {
        m_rangeText = parseBuffer(m_downloader->buffer());
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

    return fortManager->saveOriginConf(successMessage());
#else
    Q_UNUSED(fortManager)

    return true;
#endif
}

QString TaskTasix::parseBuffer(const QByteArray &buffer) const
{
    const QStringList list = parseCustomBuffer(buffer);

    if (list.isEmpty())
        return QString();

    // Merge lines
    Ip4Range ip4Range;
    if (!ip4Range.fromText(list.join('\n')))
        return QString();

    return ip4Range.toText();
}

QStringList TaskTasix::parseTasixBuffer(const QByteArray &buffer)
{
    QStringList list;

    // Parse lines
    const QString text = QString::fromLatin1(buffer);

    foreach (const QStringRef &line, text.splitRef(
                 '\n', QString::SkipEmptyParts)) {
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

    // Include local networks
    if (!list.isEmpty()) {
        list.append("10.0.0.0/8");
        list.append("127.0.0.0/8");
        list.append("169.254.0.0/16");
        list.append("172.16.0.0/12");
        list.append("192.168.0.0/16");
    }

    return list;
}
