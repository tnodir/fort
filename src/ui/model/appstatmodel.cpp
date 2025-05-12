#include "appstatmodel.h"

#include <QIcon>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfo.h>
#include <appinfo/appinfocache.h>
#include <fortmanager.h>
#include <manager/translationmanager.h>
#include <stat/statmanager.h>
#include <util/fileutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

#include "traflistmodel.h"

AppStatModel::AppStatModel(QObject *parent) : TableSqlModel(parent) { }

void AppStatModel::setUnit(TrafUnitType::TrafUnit v)
{
    if (m_unitType.unit() == v)
        return;

    m_unitType.setUnit(v);

    refreshLater();
}

void AppStatModel::setType(TrafUnitType::TrafType v)
{
    if (m_unitType.type() == v)
        return;

    m_unitType.setType(v);

    resetLater();
}

void AppStatModel::setTrafTime(qint32 v)
{
    if (m_trafTime == v)
        return;

    m_trafTime = v;

    resetLater();
}

StatManager *AppStatModel::statManager() const
{
    return IoC<StatManager>();
}

AppInfoCache *AppStatModel::appInfoCache() const
{
    return IoC<AppInfoCache>();
}

SqliteDb *AppStatModel::sqliteDb() const
{
    return statManager()->sqliteDb();
}

void AppStatModel::initialize()
{
    setSortColumn(int(AppStatColumn::Download));
    setSortOrder(Qt::DescendingOrder);

    connect(statManager(), &StatManager::appStatRemoved, this, &AppStatModel::refresh);
    connect(statManager(), &StatManager::appCreated, this, &AppStatModel::refresh);

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &AppStatModel::refresh);
}

int AppStatModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(AppStatColumn::Count);
}

QVariant AppStatModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return headerDataDisplay(section, role);

    // Icon
    case Qt::DecorationRole:
        return headerDataDecoration(section);
    }

    return {};
}

QVariant AppStatModel::headerDataDisplay(int section, int role) const
{
    if (role == Qt::DisplayRole) {
        if (section >= int(AppStatColumn::Download) && section <= int(AppStatColumn::Upload))
            return {};
    }

    return AppStatModel::columnName(AppStatColumn(section));
}

QVariant AppStatModel::headerDataDecoration(int section) const
{
    switch (AppStatColumn(section)) {
    case AppStatColumn::Download:
        return IconCache::icon(":/icons/green_down.png");
    case AppStatColumn::Upload:
        return IconCache::icon(":/icons/blue_up.png");
    }

    return {};
}

QVariant AppStatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);

    // Icon
    case Qt::DecorationRole:
        return dataDecoration(index);
    }

    return {};
}

QVariant AppStatModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();

    updateRowCache(row);

    switch (AppStatColumn(index.column())) {
    case AppStatColumn::Program: {
        if (row == 0) {
            return tr("All");
        }

        return appInfoCache()->appName(m_appStatRow.appPath);
    }
    case AppStatColumn::Download:
        return m_unitType.formatTrafUnit(m_appStatRow.inBytes);
    case AppStatColumn::Upload:
        return m_unitType.formatTrafUnit(m_appStatRow.outBytes);
    }

    return {};
}

QVariant AppStatModel::dataDecoration(const QModelIndex &index) const
{
    if (AppStatColumn(index.column()) != AppStatColumn::Program)
        return {};

    const int row = index.row();
    if (row == 0) {
        return IconCache::icon(":/icons/computer-96.png");
    }

    updateRowCache(row);

    return appInfoCache()->appIcon(m_appStatRow.appPath);
}

const AppStatRow &AppStatModel::appStatRowAt(int row) const
{
    updateRowCache(row);

    return m_appStatRow;
}

void AppStatModel::remove(int row)
{
    updateRowCache(row);

    if (Q_UNLIKELY(m_appStatRow.isNull()))
        return;

    statManager()->deleteStatApp(m_appStatRow.appId);
}

bool AppStatModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql()).vars(vars).prepareRow(stmt)) {
        m_appStatRow.invalidate();
        return false;
    }

    m_appStatRow.appId = stmt.columnInt64(0);
    m_appStatRow.confAppId = stmt.columnInt64(1);
    m_appStatRow.appPath = stmt.columnText(2);
    m_appStatRow.inBytes = stmt.columnInt64(3);
    m_appStatRow.outBytes = stmt.columnInt64(4);

    return true;
}

QString AppStatModel::sqlBase() const
{
    bool useTrafTime = true;
    QString trafTotalTable;
    QString trafAppTable;

    switch (type()) {
    case TrafUnitType::TrafHourly: {
        trafTotalTable = "traffic_hour";
        trafAppTable = "traffic_app_hour";
    } break;
    case TrafUnitType::TrafDaily: {
        trafTotalTable = "traffic_day";
        trafAppTable = "traffic_app_day";
    } break;
    case TrafUnitType::TrafMonthly: {
        trafTotalTable = "traffic_month";
        trafAppTable = "traffic_app_month";
    } break;
    case TrafUnitType::TrafTotal: {
        trafTotalTable = "(SELECT sum(in_bytes) AS in_bytes, sum(out_bytes) AS out_bytes"
                         "  FROM traffic_app)";
        trafAppTable = "traffic_app";
        useTrafTime = false;
    } break;
    }

    const auto sqlTrafTime = (useTrafTime ? ("ta.traf_time = " + QString::number(m_trafTime))
                                          : QLatin1String("1 = 1"));

    const auto sqlTotalTraf =
            QString("SELECT t.app_id, t.conf_app_id, t.path, ta.in_bytes, ta.out_bytes"
                    "  FROM (SELECT 0 AS app_id, 0 AS conf_app_id, '' AS path) t"
                    "  LEFT JOIN %1 ta ON %2")
                    .arg(trafTotalTable, sqlTrafTime);

    const auto sqlAppTraf =
            QString("SELECT t.app_id, t.conf_app_id, t.path, ta.in_bytes, ta.out_bytes"
                    "  FROM app t"
                    "  LEFT JOIN %1 ta ON ta.app_id = t.app_id AND %2")
                    .arg(trafAppTable, sqlTrafTime);

    return sqlTotalTraf + " UNION ALL " + sqlAppTraf;
}

QString AppStatModel::sqlOrderColumn() const
{
    static const QStringList orderColumns = {
        "path", // Program
        "in_bytes", // Download
        "out_bytes", // Upload
    };

    Q_ASSERT(sortColumn() >= 0 && sortColumn() < orderColumns.size());

    const auto &columnsStr = orderColumns.at(sortColumn());

    return columnsStr + sqlOrderAsc() + ", path";
}

QString AppStatModel::columnName(const AppStatColumn column)
{
    static QStringList g_columnNames;
    static int g_language = -1;

    const int language = IoC<TranslationManager>()->language();
    if (g_language != language) {
        g_language = language;

        g_columnNames = {
            tr("Program"),
            tr("Download"),
            tr("Upload"),
        };
    }

    return g_columnNames.value(int(column));
}
