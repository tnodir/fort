#include "zonelistmodel.h"

#include <QJsonDocument>
#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/confzonemanager.h>
#include <fortglobal.h>
#include <util/conf/confutil.h>
#include <util/fileutil.h>
#include <util/json/jsonutil.h>

#include "zonesourcewrapper.h"
#include "zonetypewrapper.h"

using namespace Fort;

namespace {
const QLoggingCategory LC("model.zoneList");
}

ZoneListModel::ZoneListModel(QObject *parent) : TableSqlModel(parent) { }

SqliteDb *ZoneListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void ZoneListModel::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();
    auto confZoneManager = Fort::dependency<ConfZoneManager>();

    setupZoneTypes();
    setupZoneSources();

    connect(confManager, &ConfManager::confChanged, this, &ZoneListModel::refresh);

    connect(confZoneManager, &ConfZoneManager::zoneAdded, this, &TableItemModel::reset);
    connect(confZoneManager, &ConfZoneManager::zoneRemoved, this, &TableItemModel::reset);
    connect(confZoneManager, &ConfZoneManager::zoneUpdated, this, &TableItemModel::refresh);
}

int ZoneListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 5;
}

QVariant ZoneListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        return headerDataDisplay(section);
    }
    return {};
}

QVariant ZoneListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);

    // Enabled
    case Qt::CheckStateRole:
        return dataCheckState(index);
    }

    return {};
}

QVariant ZoneListModel::headerDataDisplay(int section) const
{
    switch (section) {
    case 0:
        return tr("Zone");
    case 1:
        return tr("Source");
    case 2:
        return tr("Addresses");
    case 3:
        return tr("Last Download");
    case 4:
        return tr("Last Success");
    }
    return {};
}

QVariant ZoneListModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &zoneRow = zoneRowAt(row);

    switch (column) {
    case 0:
        return QString("%1) %2").arg(QString::number(zoneRow.zoneId), zoneRow.zoneName);
    case 1: {
        const auto zoneSource = ZoneSourceWrapper(zoneSourceByCode(zoneRow.sourceCode));
        return zoneSourceTitleById(zoneSource.id());
    }
    case 2:
        return zoneRow.addressCount;
    case 3:
        return zoneRow.lastRun;
    case 4:
        return zoneRow.lastSuccess;
    }

    return {};
}

QVariant ZoneListModel::dataCheckState(const QModelIndex &index) const
{
    if (index.column() == 0) {
        const auto &zoneRow = zoneRowAt(index.row());
        return zoneRow.enabled ? Qt::Checked : Qt::Unchecked;
    }

    return {};
}

bool ZoneListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value);

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto &zoneRow = zoneRowAt(index.row());
        return confZoneManager()->updateZoneEnabled(zoneRow.zoneId, !zoneRow.enabled);
    }

    return false;
}

Qt::ItemFlags ZoneListModel::flagIsUserCheckable(const QModelIndex &index) const
{
    return index.column() == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags;
}

const ZoneRow &ZoneListModel::zoneRowAt(int row) const
{
    updateRowCache(row);

    return m_zoneRow;
}

QVariant ZoneListModel::zoneTypeById(int typeId) const
{
    return zoneTypes().value(typeId);
}

QVariant ZoneListModel::zoneTypeByCode(const QString &typeCode) const
{
    const int typeId = ZoneTypeWrapper::idByCode(typeCode);
    return zoneTypeById(typeId);
}

QVariant ZoneListModel::zoneSourceById(int sourceId) const
{
    return zoneSources().value(sourceId);
}

QVariant ZoneListModel::zoneSourceByCode(const QString &sourceCode) const
{
    const int sourceId = ZoneSourceWrapper::idByCode(sourceCode);
    return zoneSourceById(sourceId);
}

bool ZoneListModel::updateTableRow(const QVariantHash &vars, int /*row*/) const
{
    return updateZoneRow(sql(), vars, m_zoneRow);
}

bool ZoneListModel::updateZoneRow(
        const QString &sql, const QVariantHash &vars, ZoneRow &zoneRow) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars(vars).prepareRow(stmt))
        return false;

    zoneRow.zoneId = stmt.columnInt(0);
    zoneRow.enabled = stmt.columnBool(1);
    zoneRow.customUrl = stmt.columnBool(2);
    zoneRow.zoneName = stmt.columnText(3);
    zoneRow.sourceCode = stmt.columnText(4);
    zoneRow.url = stmt.columnText(5);
    zoneRow.formData = stmt.columnText(6);
    zoneRow.addressCount = stmt.columnInt(7);
    zoneRow.textInline = stmt.columnText(8);
    zoneRow.textChecksum = stmt.columnText(9);
    zoneRow.binChecksum = stmt.columnText(10);
    zoneRow.sourceModTime = stmt.columnDateTime(11);
    zoneRow.lastRun = stmt.columnDateTime(12);
    zoneRow.lastSuccess = stmt.columnDateTime(13);

    return true;
}

QString ZoneListModel::sqlBase() const
{
    return "SELECT"
           "    zone_id,"
           "    enabled,"
           "    custom_url,"
           "    name,"
           "    source_code,"
           "    url,"
           "    form_data,"
           "    address_count,"
           "    text_inline,"
           "    text_checksum,"
           "    bin_checksum,"
           "    source_modtime,"
           "    last_run,"
           "    last_success"
           "  FROM zone";
}

void ZoneListModel::setupZoneTypes()
{
    const auto data = FileUtil::readFileData(":/zone/types.json");
    if (data.isEmpty())
        return;

    QString errorString;
    const auto zoneTypes = JsonUtil::jsonToVariant(data, errorString).toList();
    if (!errorString.isEmpty()) {
        qCWarning(LC) << "Zone Types: JSON error:" << errorString;
        return;
    }

    int index = 0;
    for (const auto &typeVar : zoneTypes) {
        ZoneTypeWrapper zoneType(typeVar);
        zoneType.setId(index++);
        m_zoneTypes.append(zoneType.map());
    }
}

void ZoneListModel::setupZoneSources()
{
    const auto data = FileUtil::readFileData(":/zone/sources.json");
    if (data.isEmpty())
        return;

    QString errorString;
    const auto zoneSources = JsonUtil::jsonToVariant(data, errorString).toList();
    if (!errorString.isEmpty()) {
        qCWarning(LC) << "Zone Sources: JSON error:" << errorString;
        return;
    }

    int index = 0;
    for (const auto &sourceVar : zoneSources) {
        ZoneSourceWrapper zoneSource(sourceVar);
        zoneSource.setId(index++);
        m_zoneSources.append(zoneSource.map());
    }
}

QString ZoneListModel::zoneSourceTitleById(const int sourceId)
{
    static const char *const sourceTitles[] = {
        QT_TR_NOOP("Addresses from Inline Text"),
        QT_TR_NOOP("Addresses from Local File"),
        QT_TR_NOOP("WindowsSpyBlocker"),
        QT_TR_NOOP("FireHOL Level-1"),
        QT_TR_NOOP("TAS-IX Addresses"),
    };

    if (sourceId >= 0 && sourceId < std::size(sourceTitles)) {
        return tr(sourceTitles[sourceId]);
    }

    return {};
}
