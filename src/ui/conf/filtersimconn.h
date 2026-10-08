#ifndef FILTERSIMCONN_H
#define FILTERSIMCONN_H

#include <QString>

#include <driver/drivercommon.h>

struct FilterSimConn
{
    DriverCommon::ConnFilterResult result = DriverCommon::ConnFilterAllowed;

    // The input: direction, IP version, profile, loopback, protocol, addresses and ports.
    // The results: reason, rule_id, act.zone_id and app.data.
    FORT_CONF_META_CONN conn {};

    QString appPath;
};

#endif // FILTERSIMCONN_H
