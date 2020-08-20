#include "quotamanager.h"

#include "../fortsettings.h"

QuotaManager::QuotaManager(FortSettings *fortSettings, QObject *parent) :
    QObject(parent), m_fortSettings(fortSettings)
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

        emit alert(tr("Day traffic quota exceeded!"));
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

        emit alert(tr("Month traffic quota exceeded!"));
    }
}

qint32 QuotaManager::quotaDayAlerted() const
{
    return m_fortSettings->quotaDayAlerted();
}

void QuotaManager::setQuotaDayAlerted(qint32 v)
{
    m_quotaDayAlerted = v;

    m_fortSettings->setQuotaDayAlerted(v);
}

qint32 QuotaManager::quotaMonthAlerted() const
{
    return m_fortSettings->quotaMonthAlerted();
}

void QuotaManager::setQuotaMonthAlerted(qint32 v)
{
    m_quotaMonthAlerted = v;

    m_fortSettings->setQuotaMonthAlerted(v);
}
