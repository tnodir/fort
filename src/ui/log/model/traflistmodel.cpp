#include "traflistmodel.h"

#include "../../db/databasemanager.h"
#include "../../db/databasesql.h"
#include "../../util/dateutil.h"

TrafListModel::TrafListModel(DatabaseManager *databaseManager,
                             QObject *parent) :
    QAbstractListModel(parent),
    m_type(TrafHourly),
    m_appId(0),
    m_minTrafTime(0),
    m_maxTrafTime(0),
    m_databaseManager(databaseManager)
{
}

void TrafListModel::setType(TrafListModel::TrafType type)
{
    m_type = type;
}

void TrafListModel::setAppPath(const QString &appPath)
{
    m_appPath = appPath;
}

void TrafListModel::reset()
{
    m_appId = m_appPath.isEmpty()
            ? 0 : m_databaseManager->getAppId(m_appPath);

    const char *sqlMinTrafTime = getSqlMinTrafTime(m_type, m_appId);

    if (sqlMinTrafTime) {
        m_minTrafTime = m_databaseManager->getMinTrafTime(
                    sqlMinTrafTime, m_appId);

        m_maxTrafTime = getMaxTrafTime(m_type);

        m_trafCount = getTrafCount(m_type, m_minTrafTime, m_minTrafTime);
    }
}

int TrafListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_trafCount;
}

QVariant TrafListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role >= DateTimeRole && role <= SumRole) {
        const int row = index.row();

        return QVariant();
    }
    return QVariant();
}

QHash<int, QByteArray> TrafListModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {DateTimeRole, "dateTime"},
        {DownloadRole, "download"},
        {UploadRole, "upload"},
        {SumRole, "sum"}
    };

    return roles;
}

void TrafListModel::clear()
{
}

qint32 TrafListModel::getMaxTrafTime(TrafType type)
{
    const qint64 unixTime = DateUtil::getUnixTime();

    switch (type) {
    case TrafHourly: return DateUtil::getUnixHour(unixTime);
    case TrafDaily: return DateUtil::getUnixDay(unixTime);
    case TrafMonthly: return DateUtil::getUnixMonth(unixTime);
    case TrafTotal: break;
    }
    return 0;
}

qint32 TrafListModel::getTrafCount(TrafType type, qint32 minTrafTime,
                                   qint32 maxTrafTime)
{
    const qint32 hours = maxTrafTime - minTrafTime + 1;
    if (type == TrafHourly)
        return hours;

    const qint32 days = hours / 24;
    if (type == TrafDaily)
        return days;

    const qint32 months = days / 30;
    if (type == TrafMonthly)
        return months;

    return 0;
}

const char *TrafListModel::getSqlMinTrafTime(TrafType type, qint64 appId)
{
    switch (type) {
    case TrafHourly: return appId ? DatabaseSql::sqlSelectMinTrafAppHour
                                  : DatabaseSql::sqlSelectMinTrafHour;
    case TrafDaily: return appId ? DatabaseSql::sqlSelectMinTrafAppDay
                                 : DatabaseSql::sqlSelectMinTrafDay;
    case TrafMonthly: return appId ? DatabaseSql::sqlSelectMinTrafAppMonth
                                   : DatabaseSql::sqlSelectMinTrafMonth;
    case TrafTotal: break;
    }
    return nullptr;
}

const char *TrafListModel::getSqlTrafBytes(TrafType type, qint64 appId)
{
    switch (type) {
    case TrafHourly: return appId ? DatabaseSql::sqlSelectTrafAppHour
                                  : DatabaseSql::sqlSelectTrafHour;
    case TrafDaily: return appId ? DatabaseSql::sqlSelectTrafAppDay
                                 : DatabaseSql::sqlSelectTrafDay;
    case TrafMonthly: return appId ? DatabaseSql::sqlSelectTrafAppMonth
                                   : DatabaseSql::sqlSelectTrafMonth;
    case TrafTotal: return appId ? DatabaseSql::sqlSelectTrafAppTotal
                                 : DatabaseSql::sqlSelectTrafTotal;
    }
    return nullptr;
}
