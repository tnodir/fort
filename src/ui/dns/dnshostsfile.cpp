#include "dnshostsfile.h"

#include <QDataStream>
#include <QDateTime>
#include <QHash>
#include <QIODevice>

#include <util/fileutil.h>
#include <util/osutil.h>

// The file: the magic, the format's version, the write time (msecs since the epoch), then the
// sections: a tag, the data's size and the data. A reader skips an unknown section, so a later
// version adds its data as a section of a new tag (a changed layout of a section too).
//
// The processes' names section: the names' count and the names (UTF-8), then the entries' count
// and the entries: the process id, the IPv6 flag, the address (an IPv4 one as quint32, an IPv6 one
// as its 16 bytes), the name's index and the expiration time (msecs since the epoch).

namespace {

inline constexpr quint32 fileMagic = 0x46444853; // "FDHS"
inline constexpr quint16 fileVersion = 1;

enum SectionTag : quint16 {
    SectionProcessHosts = 1,
};

void setupStream(QDataStream &stream)
{
    stream.setVersion(QDataStream::Qt_6_0);
}

struct NamesTable
{
    quint32 index(const QString &name)
    {
        auto it = indexes.constFind(name);
        if (it == indexes.constEnd()) {
            it = indexes.insert(name, quint32(names.size()));
            names.append(name.toUtf8());
        }

        return *it;
    }

    QHash<QString, quint32> indexes;
    QList<QByteArray> names;
};

void writeIp(QDataStream &stream, const ProcessIpKey &key)
{
    if (key.isIPv6) {
        stream.writeRawData(key.ip.data, sizeof(ip6_addr_t));
    } else {
        stream << quint32(key.ip.v4);
    }
}

QByteArray processHostsData(const DnsHostsHash &hosts, qint64 nowMsecs)
{
    NamesTable namesTable;
    quint32 entryCount = 0;

    QByteArray entries;
    QDataStream entriesStream(&entries, QDataStream::WriteOnly);
    setupStream(entriesStream);

    hosts.forEachUnexpired(
            [&](const ProcessIpKey &key, const QString &name, qint64 remainingMsecs) {
                entriesStream << key.pid << quint8(key.isIPv6);
                writeIp(entriesStream, key);
                entriesStream << namesTable.index(name) << qint64(nowMsecs + remainingMsecs);

                ++entryCount;
            });

    QByteArray data;
    QDataStream stream(&data, QDataStream::WriteOnly);
    setupStream(stream);

    stream << quint32(namesTable.names.size());
    for (const QByteArray &name : std::as_const(namesTable.names)) {
        stream << name;
    }

    stream << entryCount;
    stream.writeRawData(entries.constData(), int(entries.size()));

    return data;
}

void writeSection(QDataStream &stream, quint16 tag, const QByteArray &data)
{
    stream << tag << quint32(data.size());
    stream.writeRawData(data.constData(), int(data.size()));
}

bool readHeader(QDataStream &stream)
{
    quint32 magic = 0;
    quint16 version = 0;
    qint64 writeMsecs = 0;

    stream >> magic >> version >> writeMsecs;

    if (stream.status() != QDataStream::Ok)
        return false;

    if (magic != fileMagic || version != fileVersion)
        return false;

    return writeMsecs >= OsUtil::bootTime().toMSecsSinceEpoch();
}

ip_addr_t readIp(QDataStream &stream, bool isIPv6)
{
    ip_addr_t ip = {};

    if (isIPv6) {
        stream.readRawData(ip.data, sizeof(ip6_addr_t));
    } else {
        quint32 ip4 = 0;
        stream >> ip4;
        ip.v4 = ip4;
    }

    return ip;
}

void readProcessHost(
        QDataStream &stream, const QStringList &names, DnsHostsHash &hosts, qint64 nowMsecs)
{
    quint32 pid = 0;
    quint8 isIPv6 = 0;
    stream >> pid >> isIPv6;

    const ip_addr_t ip = readIp(stream, isIPv6 != 0);

    quint32 nameIndex = 0;
    qint64 expireMsecs = 0;
    stream >> nameIndex >> expireMsecs;

    if (stream.status() != QDataStream::Ok || qsizetype(nameIndex) >= names.size()) {
        stream.setStatus(QDataStream::ReadCorruptData);
        return;
    }

    const qint64 remainingMsecs = expireMsecs - nowMsecs;
    if (remainingMsecs <= 0)
        return;

    hosts.insert(ProcessIpKey(pid, ip, isIPv6 != 0), names.at(nameIndex),
            QDeadlineTimer(remainingMsecs));
}

bool readProcessHosts(const QByteArray &data, DnsHostsHash &hosts, qint64 nowMsecs)
{
    QDataStream stream(data);
    setupStream(stream);

    quint32 nameCount = 0;
    stream >> nameCount;

    QStringList names;
    for (quint32 i = 0; i < nameCount && stream.status() == QDataStream::Ok; ++i) {
        QByteArray name;
        stream >> name;
        names.append(QString::fromUtf8(name));
    }

    quint32 entryCount = 0;
    stream >> entryCount;

    for (quint32 i = 0; i < entryCount && stream.status() == QDataStream::Ok; ++i) {
        readProcessHost(stream, names, hosts, nowMsecs);
    }

    return stream.status() == QDataStream::Ok;
}

bool readSection(QDataStream &stream, DnsHostsHash &hosts, qint64 nowMsecs)
{
    quint16 tag = 0;
    quint32 size = 0;
    stream >> tag >> size;

    if (stream.status() != QDataStream::Ok || size > stream.device()->bytesAvailable())
        return false;

    QByteArray data(size, Qt::Uninitialized);
    stream.readRawData(data.data(), int(size));

    // A later version's section is skipped
    if (tag != SectionProcessHosts)
        return true;

    return readProcessHosts(data, hosts, nowMsecs);
}

}

namespace DnsHostsFile {

bool write(const QString &filePath, const DnsHostsHash &hosts)
{
    const qint64 nowMsecs = QDateTime::currentMSecsSinceEpoch();

    QByteArray data;
    QDataStream stream(&data, QDataStream::WriteOnly);
    setupStream(stream);

    stream << fileMagic << fileVersion << nowMsecs;

    writeSection(stream, SectionProcessHosts, processHostsData(hosts, nowMsecs));

    return FileUtil::writeFileData(filePath, data);
}

bool read(const QString &filePath, DnsHostsHash &hosts)
{
    const QByteArray data = FileUtil::readFileData(filePath);

    QDataStream stream(data);
    setupStream(stream);

    if (!readHeader(stream))
        return false;

    const qint64 nowMsecs = QDateTime::currentMSecsSinceEpoch();

    DnsHostsHash readHosts = hosts;

    while (!stream.atEnd()) {
        if (!readSection(stream, readHosts, nowMsecs))
            return false;
    }

    hosts = readHosts;

    return true;
}

}
