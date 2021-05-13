#include "traflistmodel.h"

#include <QLocale>

#include "../stat/statmanager.h"
#include "../stat/statsql.h"
#include "../util/dateutil.h"
#include "../util/net/netutil.h"

TrafListModel::TrafListModel(StatManager *statManager, QObject *parent) :
    TableItemModel(parent), m_statManager(statManager)
{
    connect(m_statManager, &StatManager::trafficCleared, this, &TrafListModel::resetTraf);
    connect(m_statManager, &StatManager::appTrafTotalsResetted, this, &TrafListModel::resetTraf);
}

void TrafListModel::setUnit(TrafListModel::TrafUnit unit)
{
    m_unit = unit;
}

void TrafListModel::setType(TrafListModel::TrafType type)
{
    m_type = type;
}

void TrafListModel::setAppId(qint64 appId)
{
    m_appId = appId;
}

int TrafListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_trafCount;
}

int TrafListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant TrafListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        switch (section) {
        case 0:
            return tr("Date");
        case 1:
            return tr("Download");
        case 2:
            return tr("Upload");
        case 3:
            return tr("Sum");
        }
    }
    return QVariant();
}

QVariant TrafListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        const int row = index.row();
        const int column = index.column();

        updateRowCache(row);

        switch (column) {
        case 0:
            return formatTrafTime(m_trafRow.trafTime);
        case 1:
            return formatTrafUnit(m_trafRow.inBytes);
        case 2:
            return formatTrafUnit(m_trafRow.outBytes);
        case 3:
            return formatTrafUnit(m_trafRow.inBytes + m_trafRow.outBytes);
        }
    }

    return QVariant();
}

void TrafListModel::clear()
{
    statManager()->clearTraffic();
}

void TrafListModel::resetAppTotals()
{
    statManager()->resetAppTrafTotals();
}

void TrafListModel::resetTraf()
{
    const char *sqlMinTrafTime = getSqlMinTrafTime(m_type, m_appId);

    beginResetModel();

    m_minTrafTime = statManager()->getTrafficTime(sqlMinTrafTime, m_appId);

    m_maxTrafTime = getMaxTrafTime(m_type);

    m_isEmpty = (m_minTrafTime == 0);

    if (m_minTrafTime == 0) {
        m_minTrafTime = m_maxTrafTime;
    }

    m_trafCount = getTrafCount(m_type, m_minTrafTime, m_maxTrafTime);

    invalidateRowCache();

    endResetModel();
}

void TrafListModel::reset()
{
    if (m_isEmpty) {
        resetTraf();
    } else {
        TableItemModel::reset();
    }
}

bool TrafListModel::updateTableRow(int row) const
{
    m_trafRow.trafTime = getTrafTime(row);

    const char *sqlSelectTraffic = getSqlSelectTraffic(m_type, m_appId);

    statManager()->getTraffic(
            sqlSelectTraffic, m_trafRow.trafTime, m_trafRow.inBytes, m_trafRow.outBytes, m_appId);

    return true;
}

QString TrafListModel::formatTrafUnit(qint64 bytes) const
{
    static const QVector<qint64> unitMults = {
        1, // Adaptive
        1, // Bytes
        1024, // KB
        1024 * 1024, // MB
        qint64(1024) * 1024 * 1024, // GB
        qint64(1024) * 1024 * 1024 * 1024 // TB
    };

    if (bytes == 0) {
        return QLatin1String("0");
    }

    const int trafPrec = (unit() == UnitBytes) ? 0 : 2;

    if (unit() == UnitAdaptive) {
        return NetUtil::formatDataSize(bytes, trafPrec);
    }

    const qint64 unitMult = unitMults.at(unit());

    return QLocale::c().toString(qreal(bytes) / unitMult, 'f', trafPrec);
}

QString TrafListModel::formatTrafTime(qint32 trafTime) const
{
    const qint64 unixTime = DateUtil::toUnixTime(trafTime);

    switch (m_type) {
    case TrafTotal:
        Q_FALLTHROUGH();
    case TrafHourly:
        return DateUtil::formatHour(unixTime);
    case TrafDaily:
        return DateUtil::formatDay(unixTime);
    case TrafMonthly:
        return DateUtil::formatMonth(unixTime);
    }
    return QString();
}

qint32 TrafListModel::getTrafTime(int row) const
{
    switch (m_type) {
    case TrafHourly:
        return m_maxTrafTime - row;
    case TrafDaily:
        return m_maxTrafTime - row * 24;
    case TrafMonthly:
        return DateUtil::addUnixMonths(m_maxTrafTime, -row);
    case TrafTotal:
        return m_minTrafTime;
    }
    return 0;
}

qint32 TrafListModel::getTrafCount(TrafType type, qint32 minTrafTime, qint32 maxTrafTime)
{
    if (type == TrafTotal)
        return 1;

    const qint32 hours = maxTrafTime - minTrafTime + 1;
    if (type == TrafHourly)
        return hours;

    const qint32 days = hours / 24 + 1;
    if (type == TrafDaily)
        return days;

    const qint32 months = days / 30 + 1;
    if (type == TrafMonthly)
        return months;

    Q_UNREACHABLE();
    return 0;
}

qint32 TrafListModel::getMaxTrafTime(TrafType type)
{
    const qint64 unixTime = DateUtil::getUnixTime();

    switch (type) {
    case TrafTotal:
        Q_FALLTHROUGH();
    case TrafHourly:
        return DateUtil::getUnixHour(unixTime);
    case TrafDaily:
        return DateUtil::getUnixDay(unixTime);
    case TrafMonthly:
        return DateUtil::getUnixMonth(unixTime);
    }

    Q_UNREACHABLE();
    return 0;
}

const char *TrafListModel::getSqlMinTrafTime(TrafType type, qint64 appId)
{
    switch (type) {
    case TrafHourly:
        return appId ? StatSql::sqlSelectMinTrafAppHour : StatSql::sqlSelectMinTrafHour;
    case TrafDaily:
        return appId ? StatSql::sqlSelectMinTrafAppDay : StatSql::sqlSelectMinTrafDay;
    case TrafMonthly:
        return appId ? StatSql::sqlSelectMinTrafAppMonth : StatSql::sqlSelectMinTrafMonth;
    case TrafTotal:
        return appId ? StatSql::sqlSelectMinTrafAppTotal : StatSql::sqlSelectMinTrafTotal;
    }

    Q_UNREACHABLE();
    return nullptr;
}

const char *TrafListModel::getSqlSelectTraffic(TrafType type, qint64 appId)
{
    switch (type) {
    case TrafHourly:
        return appId ? StatSql::sqlSelectTrafAppHour : StatSql::sqlSelectTrafHour;
    case TrafDaily:
        return appId ? StatSql::sqlSelectTrafAppDay : StatSql::sqlSelectTrafDay;
    case TrafMonthly:
        return appId ? StatSql::sqlSelectTrafAppMonth : StatSql::sqlSelectTrafMonth;
    case TrafTotal:
        return appId ? StatSql::sqlSelectTrafAppTotal : StatSql::sqlSelectTrafTotal;
    }

    Q_UNREACHABLE();
    return nullptr;
}
