#include "taskzonedownloader.h"

#include <QRegularExpression>

#include "../util/net/ip4range.h"
#include "../util/net/netdownloader.h"

TaskZoneDownloader::TaskZoneDownloader(QObject *parent) :
    TaskDownloader(parent)
{
}

void TaskZoneDownloader::setupDownloader()
{
    downloader()->setUrl(url());
    downloader()->setData(formData().toUtf8());

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

QStringList TaskZoneDownloader::parseAddresses(const QByteArray &buffer) const
{
    QStringList list;

    // Parse lines
    const QString text = QString::fromLatin1(buffer);

    QRegularExpression re(pattern());

    for (const QStringRef &line : text.splitRef(
                 '\n', QString::SkipEmptyParts)) {
        const auto match = re.match(line);
        if (!match.hasMatch())
            continue;

        const QString ip = match.captured(1);
        if (ip.isEmpty())
            continue;

        list.append(ip);
    }

    return list;
}
