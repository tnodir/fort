#include "quotamanager.h"

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortglobal.h>
#include <stat/statmanager.h>
#include <util/dateutil.h>

#include "statsql.h"

using namespace Fort;

QuotaManager::QuotaManager(QObject *parent) : QObject(parent) { }

void QuotaManager::setQuotaMBytes(AlertType alertType, qint64 mBytes)
{
    const qint64 bytes = mBytes * 1024 * 1024;

    auto &qi = quotaInfo(alertType);

    if (qi.quotaBytes == bytes)
        return;

    if (qi.quotaBytes != 0) {
        setQuotaAlerted(alertType, 0);
    }

    qi.quotaBytes = bytes;
}

void QuotaManager::setTrafBytes(AlertType alertType, qint64 bytes)
{
    auto &qi = quotaInfo(alertType);

    qi.trafBytes = bytes;
}

void QuotaManager::setUp()
{
    setupConfManager();
}

void QuotaManager::clear()
{
    clear(AlertDay);
    clear(AlertMonth);
}

void QuotaManager::clear(AlertType alertType)
{
    setTrafBytes(alertType, 0);

    setQuotaAlerted(alertType, 0);
}

void QuotaManager::addTraf(qint64 bytes)
{
    addQuotaTraf(AlertDay, bytes);
    addQuotaTraf(AlertMonth, bytes);
}

void QuotaManager::addQuotaTraf(AlertType alertType, qint64 bytes)
{
    auto &qi = quotaInfo(alertType);

    qi.trafBytes += bytes;
}

void QuotaManager::checkQuota(AlertType alertType, qint32 trafAt)
{
    auto &qi = quotaInfo(alertType);

    if (qi.quotaBytes == 0)
        return;

    if (qi.quotaAlerted == 0) {
        qi.quotaAlerted = quotaAlerted(alertType);
    }

    if (qi.quotaAlerted == trafAt)
        return;

    if (qi.trafBytes > qi.quotaBytes) {
        setQuotaAlerted(alertType, trafAt);

        processQuotaExceed(alertType);
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

qint32 QuotaManager::quotaAlerted(AlertType alertType) const
{
    auto &ini = Fort::ini();

    return quotaAlertedByIni(alertType, ini);
}

void QuotaManager::setQuotaAlerted(AlertType alertType, qint32 v)
{
    auto &qi = quotaInfo(alertType);
    qi.quotaAlerted = v;

    auto &ini = Fort::ini();

    const int value = quotaAlertedByIni(alertType, ini);
    if (value != v) {
        setQuotaAlertedByIni(alertType, v, ini);
        confManager()->saveIni();
    }
}

int QuotaManager::quotaAlertedByIni(AlertType alertType, IniOptions &ini)
{
    return ini.valueInt(quotaAlertedIniKey(alertType));
}

void QuotaManager::setQuotaAlertedByIni(AlertType alertType, qint32 v, IniOptions &ini)
{
    ini.setValue(quotaAlertedIniKey(alertType), v);
}

QString QuotaManager::quotaAlertedIniKey(AlertType alertType)
{
    switch (alertType) {
    case AlertDay:
        return IniOptions::quotaDayAlertedKey();
    case AlertMonth:
        return IniOptions::quotaMonthAlertedKey();
    default:
        Q_UNREACHABLE();
        return {};
    };
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

    setQuotaMBytes(AlertDay, ini.quotaDayMb());
    setQuotaMBytes(AlertMonth, ini.quotaMonthMb());

    const qint64 unixTime = DateUtil::getUnixTime();
    const qint32 trafDay = DateUtil::getUnixDay(unixTime);
    const qint32 trafMonth = DateUtil::getUnixMonth(unixTime, ini.monthStart());

    auto statManager = Fort::statManager();

    qint64 inBytes, outBytes;

    statManager->getTraffic(StatSql::sqlSelectTrafDay, trafDay, inBytes, outBytes);
    setTrafBytes(AlertDay, inBytes);

    statManager->getTraffic(StatSql::sqlSelectTrafMonth, trafMonth, inBytes, outBytes);
    setTrafBytes(AlertMonth, inBytes);
}
