#ifndef FILTERSIMULATOR_H
#define FILTERSIMULATOR_H

#include <QString>

#include <driver/drivercommon.h>
#include <util/conf/confbuffer.h>
#include <util/net/hostmatcher.h>

#include "filtersimconn.h"

// Filters a connection by the conf, Zones, Rules and Groups written as for the driver
class FilterSimulator
{
public:
    QString errorMessage() const { return m_errorMessage; }

    bool writeConf();

    void simulate(FilterSimConn &simConn) const;

    // Whether the result changes when the remote IP is of all host() filters' names and Hosts
    // Zones
    static bool isConnHostDependent(DriverCommon::ConnFilterConf cf, const FilterSimConn &simConn,
            const FORT_CONF_APP_FIND_RESULT &app, const HostMatch &allHostMatch);

private:
    bool writeRules();

    DriverCommon::ConnFilterConf connFilterConf() const;

private:
    QString m_errorMessage;

    // The names and the Hosts Zones, which the driver adds the IPs to; empty while the DNS
    // Forwarder doesn't receive the system's queries
    HostMatch m_allHostMatch;

    ConfBuffer m_confBuf;
    ConfBuffer m_zonesBuf;
    ConfBuffer m_rulesBuf;
    ConfBuffer m_groupsBuf;
};

#endif // FILTERSIMULATOR_H
