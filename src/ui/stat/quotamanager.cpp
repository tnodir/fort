#include "quotamanager.h"

#include "../conf/confmanager.h"
#include "../conf/firewallconf.h"

QuotaManager::QuotaManager(ConfManager *confManager, QObject *parent) :
    QObject(parent), m_confManager(confManager)
{
}

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

        emit alert(AlertDay);
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

        emit alert(AlertMonth);
    }
}

FirewallConf *QuotaManager::conf() const
{
    return confManager()->conf();
}

IniOptions *QuotaManager::ini() const
{
    return &conf()->ini();
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

int QuotaManager::quotaDayAlerted() const
{
    return ini()->quotaDayAlerted();
}

void QuotaManager::setQuotaDayAlerted(int v)
{
    m_quotaDayAlerted = v;

    if (ini()->quotaDayAlerted() != v) {
        ini()->setQuotaDayAlerted(v);
        confManager()->saveIni();
    }
}

int QuotaManager::quotaMonthAlerted() const
{
    return ini()->quotaMonthAlerted();
}

void QuotaManager::setQuotaMonthAlerted(int v)
{
    m_quotaMonthAlerted = v;

    if (ini()->quotaMonthAlerted() != v) {
        ini()->setQuotaMonthAlerted(v);
        confManager()->saveIni();
    }
}
