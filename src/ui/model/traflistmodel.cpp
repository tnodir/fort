#include "traflistmodel.h"

#include <QLocale>

#include <stat/statmanager.h>
#include <stat/statsql.h>
#include <util/dateutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

namespace {

bool checkTrafType(TrafUnitType::TrafType type)
{
    if (type >= TrafUnitType::TrafHourly && type <= TrafUnitType::TrafTotal)
        return true;

    Q_UNREACHABLE();
    return false;
}

static const char *const sqlSelectMinTrafApps[] = {
    StatSql::sqlSelectMinTrafAppHour,
    StatSql::sqlSelectMinTrafAppDay,
    StatSql::sqlSelectMinTrafAppMonth,
    StatSql::sqlSelectMinTrafAppTotal,
};

static const char *const sqlSelectMinTrafs[] = {
    StatSql::sqlSelectMinTrafHour,
    StatSql::sqlSelectMinTrafDay,
    StatSql::sqlSelectMinTrafMonth,
    StatSql::sqlSelectMinTrafTotal,
};

const char *getSqlMinTrafTime(TrafUnitType::TrafType type, qint64 appId)
{
    if (!checkTrafType(type))
        return nullptr;

    return (appId != 0 ? sqlSelectMinTrafApps : sqlSelectMinTrafs)[type];
}

static const char *const sqlSelectTrafApps[] = {
    StatSql::sqlSelectTrafAppHour,
    StatSql::sqlSelectTrafAppDay,
    StatSql::sqlSelectTrafAppMonth,
    StatSql::sqlSelectTrafAppTotal,
};

static const char *const sqlSelectTrafs[] = {
    StatSql::sqlSelectTrafHour,
    StatSql::sqlSelectTrafDay,
    StatSql::sqlSelectTrafMonth,
    StatSql::sqlSelectTrafTotal,
};

const char *getSqlSelectTraffic(TrafUnitType::TrafType type, qint64 appId)
{
    if (!checkTrafType(type))
        return nullptr;

    return (appId != 0 ? sqlSelectTrafApps : sqlSelectTrafs)[type];
}

}

TrafListModel::TrafListModel(QObject *parent) : TableItemModel(parent) { }

void TrafListModel::setUnit(TrafUnitType::TrafUnit v)
{
    if (m_unitType.unit() == v)
        return;

    m_unitType.setUnit(v);

    refreshLater();
}

void TrafListModel::setType(TrafUnitType::TrafType v)
{
    if (m_unitType.type() == v)
        return;

    m_unitType.setType(v);

    resetLater();
}

void TrafListModel::setAppId(qint64 appId)
{
    if (m_appId == appId)
        return;

    m_appId = appId;

    resetLater();
}

StatManager *TrafListModel::statManager() const
{
    return IoC<StatManager>();
}

void TrafListModel::initialize()
{
    connect(statManager(), &StatManager::trafficCleared, this, &TrafListModel::resetLater);
    connect(statManager(), &StatManager::appTrafTotalsResetted, this, &TrafListModel::resetLater);
}

int TrafListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_trafCount;
}

int TrafListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(TrafListColumn::Count);
}

QVariant TrafListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return headerDataDisplay(section);

    // Icon
    case Qt::DecorationRole:
        return headerDataDecoration(section);
    }

    return {};
}

QVariant TrafListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);
    }

    return {};
}

QVariant TrafListModel::headerDataDisplay(int section) const
{
    switch (TrafListColumn(section)) {
    case TrafListColumn::Date:
        return tr("Date");
    case TrafListColumn::Download:
        return tr("Download");
    case TrafListColumn::Upload:
        return tr("Upload");
    case TrafListColumn::Sum:
        return tr("Sum");
    }

    return {};
}

QVariant TrafListModel::headerDataDecoration(int section) const
{
    switch (TrafListColumn(section)) {
    case TrafListColumn::Download:
        return IconCache::icon(":/icons/green_down.png");
    case TrafListColumn::Upload:
        return IconCache::icon(":/icons/blue_up.png");
    }

    return {};
}

QVariant TrafListModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    updateRowCache(row);

    switch (TrafListColumn(column)) {
    case TrafListColumn::Date:
        return m_unitType.formatTrafTime(m_trafRow.trafTime);
    case TrafListColumn::Download:
        return m_unitType.formatTrafUnit(m_trafRow.inBytes);
    case TrafListColumn::Upload:
        return m_unitType.formatTrafUnit(m_trafRow.outBytes);
    case TrafListColumn::Sum:
        return m_unitType.formatTrafUnit(m_trafRow.inBytes + m_trafRow.outBytes);
    }

    return {};
}

const TrafficRow &TrafListModel::trafficRowAt(int row) const
{
    updateRowCache(row);

    return m_trafRow;
}

void TrafListModel::reset()
{
    const auto type = this->type();

    const char *sqlMinTrafTime = getSqlMinTrafTime(type, m_appId);

    beginResetModel();

    m_maxTrafTime = getMaxTrafTime(type);

    m_minTrafTime = statManager()->getTrafficTime(sqlMinTrafTime, m_appId);
    if (m_minTrafTime == 0) {
        m_minTrafTime = m_maxTrafTime;
    }

    m_trafCount = getTrafCount(type, m_minTrafTime, m_maxTrafTime);

    invalidateRowCache();

    endResetModel();
}

bool TrafListModel::updateTableRow(const QVariantHash & /*vars*/, int row) const
{
    m_trafRow.trafTime = getTrafTime(row);

    const char *sqlSelectTraffic = getSqlSelectTraffic(type(), m_appId);

    statManager()->getTraffic(
            sqlSelectTraffic, m_trafRow.trafTime, m_trafRow.inBytes, m_trafRow.outBytes, m_appId);

    return true;
}

qint32 TrafListModel::getTrafTime(int row) const
{
    switch (type()) {
    case TrafUnitType::TrafHourly:
        return m_maxTrafTime - row;
    case TrafUnitType::TrafDaily:
        return m_maxTrafTime - row * 24;
    case TrafUnitType::TrafMonthly:
        return DateUtil::addUnixMonths(m_maxTrafTime, -row);
    case TrafUnitType::TrafTotal:
        return m_minTrafTime;
    }
    return 0;
}

qint32 TrafListModel::getTrafCount(
        TrafUnitType::TrafType type, qint32 minTrafTime, qint32 maxTrafTime)
{
    if (type == TrafUnitType::TrafTotal)
        return 1;

    const qint32 hours = maxTrafTime - minTrafTime + 1;
    if (type == TrafUnitType::TrafHourly)
        return hours;

    const qint32 days = hours / 24 + 1;
    if (type == TrafUnitType::TrafDaily)
        return days;

    const qint32 months = days / 30 + 1;
    if (type == TrafUnitType::TrafMonthly)
        return months;

    Q_UNREACHABLE();
    return 0;
}

qint32 TrafListModel::getMaxTrafTime(TrafUnitType::TrafType type)
{
    const qint64 unixTime = DateUtil::getUnixTime();

    switch (type) {
    case TrafUnitType::TrafTotal:
        Q_FALLTHROUGH();
    case TrafUnitType::TrafHourly:
        return DateUtil::getUnixHour(unixTime);
    case TrafUnitType::TrafDaily:
        return DateUtil::getUnixDay(unixTime);
    case TrafUnitType::TrafMonthly:
        return DateUtil::getUnixMonth(unixTime);
    }

    Q_UNREACHABLE();
    return 0;
}
