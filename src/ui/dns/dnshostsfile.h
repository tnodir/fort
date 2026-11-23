#ifndef DNSHOSTSFILE_H
#define DNSHOSTSFILE_H

#include <QString>

#include <util/expiringhash.h>
#include <util/net/processipkey.h>

using DnsHostsHash = ExpiringHash<ProcessIpKey, QString>;

// The processes' host names kept over the service's restart, with their expiration times
namespace DnsHostsFile {

bool write(const QString &filePath, const DnsHostsHash &hosts);

// Adds the file's unexpired names, all or none: nothing of a damaged file or of a run before the
// boot, as the processes' ids are reused
bool read(const QString &filePath, DnsHostsHash &hosts);

}

#endif // DNSHOSTSFILE_H
