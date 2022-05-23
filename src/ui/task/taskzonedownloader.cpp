#include "taskzonedownloader.h"

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QUrl>

#include <util/conf/confutil.h>
#include <util/fileutil.h>
#include <util/net/iprange.h>
#include <util/net/netdownloader.h>
#include <util/stringutil.h>

namespace {
const QLoggingCategory LC("task.taskZoneDownloader");
}

TaskZoneDownloader::TaskZoneDownloader(QObject *parent) :
    TaskDownloader(parent), m_zoneEnabled(false), m_sort(false)
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
        const auto text = QString::fromLatin1(downloader()->takeBuffer());
        const auto list = parseAddresses(text, textChecksum);

        if (!list.isEmpty()
                && (this->textChecksum() != textChecksum
                        || !FileUtil::fileExists(cacheFileBinPath()))) {
            setTextChecksum(textChecksum);
            success = storeAddresses(list);
            setAddressCount(success ? list.size() : 0);
        }
    }

    finish(success);
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

StringViewList TaskZoneDownloader::parseAddresses(const QString &text, QString &checksum) const
{
    StringViewList list;
    QCryptographicHash cryptoHash(QCryptographicHash::Sha256);

    // Parse lines
    const QRegularExpression re(pattern());

    const auto lines = StringUtil::tokenizeView(text, QLatin1Char('\n'), true);

    for (const auto &line : lines) {
        if (line.startsWith('#') || line.startsWith(';')) // commented line
            continue;

        const auto match = re.match(line);
        if (!match.hasMatch())
            continue;

        const auto ip = line.mid(match.capturedStart(1), match.capturedLength(1));
        list.append(ip);

        cryptoHash.addData(ip.toLatin1());
        cryptoHash.addData("\n");
    }

    checksum = QString::fromLatin1(cryptoHash.result().toHex());

    return list;
}

bool TaskZoneDownloader::storeAddresses(const StringViewList &list)
{
    IpRange ipRange;
    if (!ipRange.fromList(list, emptyNetMask(), sort())) {
        qCWarning(LC) << "TaskZoneDownloader:" << zoneName() << ":"
                      << ipRange.errorLineAndMessage();
        return false;
    }

    FileUtil::removeFile(cacheFileBinPath());

    // Store binary file
    ConfUtil confUtil;
    const int bufSize = confUtil.writeZone(ipRange, m_zoneData);
    if (bufSize == 0)
        return false;

    m_zoneData.resize(bufSize);

    const auto binData = qCompress(m_zoneData);

    const auto binChecksumData = QCryptographicHash::hash(binData, QCryptographicHash::Sha256);
    setBinChecksum(QString::fromLatin1(binChecksumData.toHex()));

    return FileUtil::writeFileData(cacheFileBinPath(), binData);
}

bool TaskZoneDownloader::loadAddresses()
{
    if (!FileUtil::fileExists(cacheFileBinPath()))
        return false;

    const auto binData = FileUtil::readFileData(cacheFileBinPath());

    const auto binChecksumData = QCryptographicHash::hash(binData, QCryptographicHash::Sha256);

    if (binChecksum() != QString::fromLatin1(binChecksumData.toHex())) {
        FileUtil::removeFile(cacheFileBinPath());
        return false;
    }

    m_zoneData = qUncompress(binData);

    return true;
}

bool TaskZoneDownloader::saveAddressesAsText(const QString &filePath)
{
    QString text;

    if (loadAddresses() && !zoneData().isEmpty()) {
        ConfUtil confUtil;
        IpRange ip4Range;
        if (!confUtil.loadZone(zoneData(), ip4Range))
            return false;

        text = ip4Range.toText();
    }

    return FileUtil::writeFile(filePath, text);
}

QString TaskZoneDownloader::cacheFileBasePath() const
{
    return cachePath() + QString::number(zoneId());
}

QString TaskZoneDownloader::cacheFileBinPath() const
{
    return cacheFileBasePath() + ".bin";
}
