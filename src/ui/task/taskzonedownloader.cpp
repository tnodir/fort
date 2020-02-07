#include "taskzonedownloader.h"

#include <QCryptographicHash>
#include <QRegularExpression>

#include "../util/conf/confutil.h"
#include "../util/fileutil.h"
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
}

void TaskZoneDownloader::downloadFinished(bool success)
{
    if (success) {
        success = false;

        QString textChecksum;
        const auto text = QString::fromLatin1(downloader()->buffer());
        const auto list = parseAddresses(text, textChecksum);

        if (!list.isEmpty()
                && (this->textChecksum() != textChecksum
                    || !FileUtil::fileExists(cacheFileBinPath()))) {
            setTextChecksum(textChecksum);

            QString binChecksum;
            success = storeAddresses(list, binChecksum);
            setBinChecksum(binChecksum);
        }
    }

    abort(success);
}

QVector<QStringRef> TaskZoneDownloader::parseAddresses(const QString &text,
                                                       QString &checksum) const
{
    QVector<QStringRef> list;
    QCryptographicHash cryptoHash(QCryptographicHash::Sha256);

    // Parse lines
    const QRegularExpression re(pattern());

    for (const auto line : text.splitRef('\n', QString::SkipEmptyParts)) {
        const auto match = re.match(line);
        if (!match.hasMatch())
            continue;

        const auto ip = line.mid(match.capturedStart(1),
                                 match.capturedLength(1));
        list.append(ip);

        cryptoHash.addData(ip.toLatin1());
        cryptoHash.addData("\n");
    }

    checksum = QString::fromLatin1(cryptoHash.result().toHex());

    return list;
}

bool TaskZoneDownloader::storeAddresses(const QVector<QStringRef> &list,
                                        QString &binChecksum) const
{
    Ip4Range ip4Range;
    if (!ip4Range.fromList(list, emptyNetMask(), sort()))
        return false;

    FileUtil::removeFile(cacheFileTextPath());
    FileUtil::removeFile(cacheFileBinPath());

    if (storeText()) {
        const auto text = ip4Range.toText();
        FileUtil::writeFile(cacheFileTextPath(), text);
    }

    QByteArray data;
    //ConfUtil::write();

    return FileUtil::writeFileData(cacheFileBinPath(), data);
}

QString TaskZoneDownloader::cacheFileBasePath() const
{
    return cachePath() + QString::number(zoneId());
}

QString TaskZoneDownloader::cacheFileBinPath() const
{
    return cacheFileBasePath() + ".bin";
}

QString TaskZoneDownloader::cacheFileTextPath() const
{
    return cacheFileBasePath() + ".txt";
}
