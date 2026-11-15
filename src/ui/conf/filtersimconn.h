#ifndef FILTERSIMCONN_H
#define FILTERSIMCONN_H

#include <QString>

#include <driver/drivercommon.h>

struct FilterSimConn
{
    enum HostNameResult : qint8 {
        HostNameNone = 0, // no host name: see hostDependent
        HostNameMatched, // the IP is of the DNS Forwarder's match of the name
        HostNameBlocked, // a Block Zone lists the name: the DNS Forwarder doesn't resolve it
        HostNameNoForwarder, // the DNS Forwarder doesn't receive the system's queries
    };

    // Set the remote address by its text; loopback for a loopback address too
    bool setRemoteIp(const QString &text, bool isLoopback = false);

    // Set the local address by its text of the remote address's IP version; empty for any
    bool setLocalIp(const QString &text);

    // Set the host name, which the program resolved to the remote IP, by its text; empty for none
    bool setHostName(const QString &text);

    DriverCommon::ConnFilterResult result = DriverCommon::ConnFilterAllowed;
    HostNameResult hostNameResult = HostNameNone;

    // The result may change by the remote IP's names, which the driver's addresses of the host()
    // filters' and the Hosts Zones' names include, unknown here
    bool hostDependent = false;

    // The input: direction, IP version, profile, loopback, the Service's, protocol, addresses and
    // ports.
    // The results: reason, rule_id, act.zone_id (of the Block Zone too) and app.data.
    FORT_CONF_META_CONN conn {};

    QString appPath;
    QString hostName;
};

#endif // FILTERSIMCONN_H
