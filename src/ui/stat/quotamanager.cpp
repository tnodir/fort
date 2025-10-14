#include "quotamanager.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortglobal.h>
#include <stat/statmanager.h>
#include <util/dateutil.h>

#include "statsql.h"

using namespace Fort;

QuotaManager::QuotaManager(QObject *parent) : QObject(parent) { }

void QuotaManager::setQuotaDayBytes(qint64 bytes)
{
    if (m_quotaDayBytes != bytes) {
        if (m_quotaDayBytes != 0) {
            setQuotaDayAlerted(0);
        }

        m_quotaDayBytes = bytes;
    }
}

void QuotaManager::setQuotaMonthBytes(qint64 bytes)
{
    if (m_quotaMonthBytes != bytes) {
        if (m_quotaMonthBytes != 0) {
            setQuotaMonthAlerted(0);
        }

        m_quotaMonthBytes = bytes;
    }
}

void QuotaManager::setTrafDayBytes(qint64 bytes)
{
    m_trafDayBytes = bytes;
}

void QuotaManager::setTrafMonthBytes(qint64 bytes)
{
    m_trafMonthBytes = bytes;
}

void QuotaManager::setUp()
{
    setupConfManager();
}

void QuotaManager::clear(bool clearDay, bool clearMonth)
{
    if (clearDay) {
        m_trafDayBytes = 0;

        setQuotaDayAlerted(0);
    }

    if (clearMonth) {
        m_trafMonthBytes = 0;

        setQuotaMonthAlerted(0);
    }
}

void QuotaManager::addTraf(qint64 bytes)
{
    m_trafDayBytes += bytes;
    m_trafMonthBytes += bytes;
}

void QuotaManager::checkQuotaDay(qint32 trafDay)
{
    if (m_quotaDayBytes == 0)
        return;

    if (m_quotaDayAlerted == 0) {
        m_quotaDayAlerted = quotaDayAlerted();
    }

    if (m_quotaDayAlerted == trafDay)
        return;

    if (m_trafDayBytes > m_quotaDayBytes) {
        setQuotaDayAlerted(trafDay);

        processQuotaExceed(AlertDay);
    }
}

void QuotaManager::checkQuotaMonth(qint32 trafMonth)
{
    if (m_quotaMonthBytes == 0)
        return;

    if (m_quotaMonthAlerted == 0) {
        m_quotaMonthAlerted = quotaMonthAlerted();
    }

    if (m_quotaMonthAlerted == trafMonth)
        return;

    if (m_trafMonthBytes > m_quotaMonthBytes) {
        setQuotaMonthAlerted(trafMonth);

        processQuotaExceed(AlertMonth);
    }
}

QString QuotaManager::alertTypeText(qint8 alertType)
{
    switch (alertType) {
    case AlertDay:
        return tr("Day traffic quota exceeded!");
    case AlertMonth:
        return tr("Month traffic quota exceeded!");
    default:
        Q_UNREACHABLE();
        return QString();
    };
}

void QuotaManager::setupConfManager()
{
    auto confManager = Fort::dependency<ConfManager>();

    connect(confManager, &ConfManager::iniChanged, this, &QuotaManager::setupByConfIni);
}

int QuotaManager::quotaDayAlerted() const
{
    return ini().quotaDayAlerted();
}

void QuotaManager::setQuotaDayAlerted(int v)
{
    auto &ini = Fort::ini();

    m_quotaDayAlerted = v;

    if (ini.quotaDayAlerted() != v) {
        ini.setQuotaDayAlerted(v);
        confManager()->saveIni();
    }
}

int QuotaManager::quotaMonthAlerted() const
{
    return ini().quotaMonthAlerted();
}

void QuotaManager::setQuotaMonthAlerted(int v)
{
    auto &ini = Fort::ini();

    m_quotaMonthAlerted = v;

    if (ini.quotaMonthAlerted() != v) {
        ini.setQuotaMonthAlerted(v);
        confManager()->saveIni();
    }
}

void QuotaManager::processQuotaExceed(AlertType alertType)
{
    auto conf = Fort::conf();

    if (ini().quotaBlockInetTraffic() && !conf->blockInetTraffic()) {
        conf->setBlockInetTraffic(true);
        confManager()->saveFlags();
    }

    emit alert(alertType);
}

void QuotaManager::setupByConfIni()
{
    const auto &ini = Fort::ini();

    setQuotaDayBytes(qint64(ini.quotaDayMb()) * 1024 * 1024);
    setQuotaMonthBytes(qint64(ini.quotaMonthMb()) * 1024 * 1024);

    const qint64 unixTime = DateUtil::getUnixTime();
    const qint32 trafDay = DateUtil::getUnixDay(unixTime);
    const qint32 trafMonth = DateUtil::getUnixMonth(unixTime, ini.monthStart());

    auto statManager = Fort::statManager();

    qint64 inBytes, outBytes;

    statManager->getTraffic(StatSql::sqlSelectTrafDay, trafDay, inBytes, outBytes);
    setTrafDayBytes(inBytes);

    statManager->getTraffic(StatSql::sqlSelectTrafMonth, trafMonth, inBytes, outBytes);
    setTrafMonthBytes(inBytes);
}
