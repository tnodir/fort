#include "taskzonedownloader.h"

#include "../util/net/ip4range.h"
#include "../util/net/netdownloader.h"
#include "../util/net/netutil.h"

TaskZoneDownloader::TaskZoneDownloader(QObject *parent) :
    TaskDownloader(parent)
{
}

void TaskZoneDownloader::setupDownloader()
{
    downloader()->setUrl("http://mrlg.tas-ix.uz/index.cgi");
    downloader()->setData("router=cisco&pass1=&query=1&arg=");

    m_rangeText = QString();
}

void TaskZoneDownloader::downloadFinished(bool success)
{
    if (success) {
        m_rangeText = parseBuffer(downloader()->buffer());
        success = !m_rangeText.isEmpty();
    }

    abort(success);
}

QString TaskZoneDownloader::parseBuffer(const QByteArray &buffer) const
{
    const QStringList list = parseAddresses(buffer);

    if (list.isEmpty())
        return QString();

    // Merge lines
    Ip4Range ip4Range;
    if (!ip4Range.fromText(list.join('\n'), 24))
        return QString();

    return ip4Range.toText();
}

QStringList TaskZoneDownloader::parseAddresses(const QByteArray &buffer)
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
