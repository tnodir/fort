#include "taskzonedownloader.h"

#include <QCryptographicHash>
#include <QRegularExpression>
#include <QUrl>

#include "../util/conf/confutil.h"
#include "../util/fileutil.h"
#include "../util/net/ip4range.h"
#include "../util/net/netdownloader.h"

TaskZoneDownloader::TaskZoneDownloader(QObject *parent) :
    TaskDownloader(parent),
    m_zoneEnabled(false),
    m_storeText(false),
    m_sort(false)
{
}

void TaskZoneDownloader::setupDownloader()
{
    if (QUrl::fromUserInput(url()).isLocalFile()) {
        // Load addresses from local file
        loadLocalFile();
        return;
    }

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

            success = storeAddresses(list);
        }
    }

    abort(success);
}

void TaskZoneDownloader::loadLocalFile()
{
    bool success = false;

    if (sourceModTime() != FileUtil::fileModTime(url())
            || !FileUtil::fileExists(cacheFileBinPath())) {
        const auto buffer = FileUtil::readFileData(url());
        downloader()->setBuffer(buffer);
        success = true;
    }

    downloadFinished(success);
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

bool TaskZoneDownloader::storeAddresses(const QVector<QStringRef> &list)
{
    Ip4Range ip4Range;
    if (!ip4Range.fromList(list, emptyNetMask(), sort()))
        return false;

    FileUtil::removeFile(cacheFileTextPath());
    FileUtil::removeFile(cacheFileBinPath());

    // Store text file
    if (storeText()) {
        const auto text = ip4Range.toText();
        FileUtil::writeFile(cacheFileTextPath(), text);
    }

    // Store binary file
    ConfUtil confUtil;
    const int bufSize = confUtil.writeZone(ip4Range, m_zoneData);
    if (bufSize == 0)
        return false;

    m_zoneData.resize(bufSize);

    const auto binData = qCompress(m_zoneData);

    const auto binChecksumData = QCryptographicHash::hash(
                binData, QCryptographicHash::Sha256);

    m_binChecksum = QString::fromLatin1(binChecksumData.toHex());

    return FileUtil::writeFileData(cacheFileBinPath(), binData);
}

bool TaskZoneDownloader::loadAddresses()
{
    if (!FileUtil::fileExists(cacheFileBinPath()))
        return false;

    const auto binData = FileUtil::readFileData(cacheFileBinPath());

    const auto binChecksumData = QCryptographicHash::hash(
                binData, QCryptographicHash::Sha256);

    if (m_binChecksum != QString::fromLatin1(binChecksumData.toHex())) {
        FileUtil::removeFile(cacheFileBinPath());
        return false;
    }

    m_zoneData = qUncompress(binData);

    return true;
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
