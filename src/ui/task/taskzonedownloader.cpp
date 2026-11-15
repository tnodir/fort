#include "taskzonedownloader.h"

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QUrl>

#include <util/conf/confbuffer.h>
#include <util/fileutil.h>
#include <util/net/hostrange.h>
#include <util/net/iprange.h>
#include <util/net/netdownloader.h>
#include <util/stringutil.h>

namespace {
const QLoggingCategory LC("task.zoneDownloader");

QString binDataChecksum(const QByteArray &binData)
{
    const auto checksumData = QCryptographicHash::hash(binData, QCryptographicHash::Sha256);

    return QString::fromLatin1(checksumData.toHex());
}

// The driver gets the Hosts Zone's addresses at runtime
QByteArray emptyZoneData()
{
    ConfBuffer confBuf;
    confBuf.writeZone(IpRange());

    return confBuf.buffer();
}
}

TaskZoneDownloader::TaskZoneDownloader(QObject *parent) : TaskDownloader(parent) { }

void TaskZoneDownloader::setupDownloader()
{
    TaskDownloader::setupDownloader();

    // Load addresses from text inline
    if (url().isEmpty()) {
        loadTextInline();
        return;
    }

    // Load addresses from local file
    if (QUrl::fromUserInput(url()).isLocalFile()) {
        loadLocalFile();
        return;
    }

    downloader()->setUrl(url());
    downloader()->setData(formData().toUtf8());
}

void TaskZoneDownloader::downloadFinished(const QByteArray &data, bool success)
{
    finish(success && storeChangedAddresses(data));
}

bool TaskZoneDownloader::storeChangedAddresses(const QByteArray &data)
{
    QString newChecksum;
    const auto text = QString::fromLatin1(data);
    const auto list = parseAddresses(text, newChecksum);

    if (list.isEmpty() || !isCacheOutdated(newChecksum))
        return false;

    setTextChecksum(newChecksum);

    const bool success = isHosts() ? storeHosts(list) : storeAddresses(list);
    setAddressCount(success ? list.size() : 0);

    return success;
}

bool TaskZoneDownloader::isCacheOutdated(const QString &newChecksum) const
{
    return textChecksum() != newChecksum || !hasCache();
}

bool TaskZoneDownloader::hasCache() const
{
    return FileUtil::fileExists(cacheFileBinPath());
}

void TaskZoneDownloader::loadTextInline()
{
    const QByteArray data = textInline().toUtf8();

    downloadFinished(data, /*success=*/true);
}

void TaskZoneDownloader::loadLocalFile()
{
    QByteArray data;
    bool success = false;

    const auto fileModTime = FileUtil::fileModTime(url());

    if (sourceModTime() != fileModTime || !hasCache()) {
        data = FileUtil::readFileData(url());
        setSourceModTime(fileModTime);
        success = true;
    }

    downloadFinished(data, success);
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

        const auto match = StringUtil::match(re, line);
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
    ipRange.setEmptyNetMask(emptyNetMask());

    if (!ipRange.fromList(list, sort())) {
        qCWarning(LC) << zoneName() << ":" << ipRange.errorLineAndMessageDetails();
        return false;
    }

    ConfBuffer confBuf;
    confBuf.writeZone(ipRange);

    m_zoneData = confBuf.buffer();

    return storeCacheData(m_zoneData);
}

bool TaskZoneDownloader::storeHosts(const StringViewList &list)
{
    HostRange hostRange;

    if (!hostRange.fromList(list)) {
        qCWarning(LC) << zoneName() << ":" << hostRange.errorLineAndMessageDetails();
        return false;
    }

    m_zoneData = emptyZoneData();

    return storeCacheData(hostRange.toText().toUtf8());
}

bool TaskZoneDownloader::loadAddresses()
{
    if (!isHosts()) {
        return loadCacheData(m_zoneData);
    }

    if (!hasCache())
        return false;

    m_zoneData = emptyZoneData();

    return true;
}

bool TaskZoneDownloader::loadHosts(HostRange &hostRange)
{
    QByteArray data;
    if (!loadCacheData(data))
        return false;

    return hostRange.fromText(QString::fromUtf8(data));
}

bool TaskZoneDownloader::saveAddressesAsText(const QString &filePath)
{
    QByteArray data;
    QString text;

    if (loadCacheData(data) && !data.isEmpty()) {
        if (!cacheDataToText(data, text))
            return false;
    }

    return FileUtil::writeFile(filePath, text);
}

bool TaskZoneDownloader::storeCacheData(const QByteArray &data)
{
    FileUtil::removeFile(cacheFileBinPath());

    if (data.isEmpty())
        return false;

    const auto binData = qCompress(data);

    setBinChecksum(binDataChecksum(binData));

    return FileUtil::writeFileData(cacheFileBinPath(), binData);
}

bool TaskZoneDownloader::loadCacheData(QByteArray &data)
{
    if (!hasCache())
        return false;

    const auto binData = FileUtil::readFileData(cacheFileBinPath());

    if (binChecksum() != binDataChecksum(binData)) {
        FileUtil::removeFile(cacheFileBinPath());
        return false;
    }

    data = qUncompress(binData);

    return true;
}

bool TaskZoneDownloader::cacheDataToText(const QByteArray &data, QString &text) const
{
    if (isHosts()) {
        text = QString::fromUtf8(data);
        return true;
    }

    ConfBuffer confBuf(data);

    IpRange ipRange;
    if (!confBuf.loadZone(ipRange))
        return false;

    text = ipRange.toText();

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
