#include "tasktasix.h"

#include "../conf/addressgroup.h"
#include "../conf/firewallconf.h"
#ifndef TASK_TEST
#include "../fortmanager.h"
#endif
#include "../util/net/ip4range.h"
#include "../util/net/netdownloader.h"
#include "../util/net/netutil.h"

TaskTasix::TaskTasix(QObject *parent) :
    TaskDownloader(parent)
{
}

void TaskTasix::setupDownloader()
{
    downloader()->setUrl("http://mrlg.tas-ix.uz/index.cgi");
    downloader()->setData("router=cisco&pass1=&query=1&arg=");

    m_rangeText = QString();
}

void TaskTasix::downloadFinished(bool success)
{
    if (success) {
        m_rangeText = parseBuffer(downloader()->buffer());
        success = !m_rangeText.isEmpty();
    }

    abort(success);
}

bool TaskTasix::processResult(FortManager *fortManager)
{
#ifndef TASK_TEST
    FirewallConf *conf = fortManager->firewallConf();
    AddressGroup *inetGroup = conf->inetAddressGroup();

    if (inetGroup->excludeText() == m_rangeText)
        return false;

    inetGroup->setExcludeText(m_rangeText);

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

    for (const QStringRef &line : text.splitRef(
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
        list.append(NetUtil::localIpv4Networks());
    }

    return list;
}
