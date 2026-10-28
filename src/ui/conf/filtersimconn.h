#ifndef FILTERSIMCONN_H
#define FILTERSIMCONN_H

#include <QString>

#include <driver/drivercommon.h>

struct FilterSimConn
{
    // Set the remote address by its text; loopback for a loopback address too
    bool setRemoteIp(const QString &text, bool isLoopback = false);

    // Set the local address by its text of the remote address's IP version; empty for any
    bool setLocalIp(const QString &text);

    DriverCommon::ConnFilterResult result = DriverCommon::ConnFilterAllowed;

    // The input: direction, IP version, profile, loopback, protocol, addresses and ports.
    // The results: reason, rule_id, act.zone_id and app.data.
    FORT_CONF_META_CONN conn {};

    QString appPath;
};

#endif // FILTERSIMCONN_H
