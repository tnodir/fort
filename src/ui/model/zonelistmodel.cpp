#include "zonelistmodel.h"

#include <QJsonDocument>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../conf/confmanager.h"
#include "../util/fileutil.h"
#include "../util/json/jsonutil.h"
#include "zonesourcewrapper.h"
#include "zonetypewrapper.h"

ZoneListModel::ZoneListModel(ConfManager *confManager,
                             QObject *parent) :
    TableSqlModel(parent),
    m_confManager(confManager)
{
}

SqliteDb *ZoneListModel::sqliteDb() const
{
    return confManager()->sqliteDb();
}

void ZoneListModel::initialize()
{
    initZoneTypes();
    initZoneSources();
}

int ZoneListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant ZoneListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal
            && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return tr("Zone");
        case 1: return tr("Source");
        case 2: return tr("Last Run");
        case 3: return tr("Last Success");
        }
    }
    return QVariant();
}

QVariant ZoneListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole: {
        const int row = index.row();
        const int column = index.column();

        const auto zoneRow = zoneRowAt(row);

        switch (column) {
        case 0: return zoneRow.zoneName;
        case 1: {
            const auto zoneSource = ZoneSourceWrapper(
                        zoneSourceByCode(zoneRow.sourceCode));
            return zoneSource.title();
        }
        case 2: return zoneRow.lastRun;
        case 3: return zoneRow.lastSuccess;
        }

        break;
    }

    case Qt::CheckStateRole:
        if (index.column() == 0) {
            const auto zoneRow = zoneRowAt(index.row());
            return zoneRow.enabled;
        }
        break;
    }

    return QVariant();
}

bool ZoneListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value)

    if (!index.isValid())
        return false;

    switch (role) {
    case Qt::CheckStateRole:
        const auto zoneRow = zoneRowAt(index.row());
        return updateZoneEnabled(zoneRow.zoneId, !zoneRow.enabled);
    }

    return false;
}

Qt::ItemFlags ZoneListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    const int column = index.column();

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren
            | (column == 0 ? Qt::ItemIsUserCheckable : Qt::NoItemFlags);
}

const ZoneRow &ZoneListModel::zoneRowAt(int row) const
{
    updateRowCache(row);

    return m_zoneRow;
}

bool ZoneListModel::addZone(const QString &zoneName, const QString &sourceCode,
                            const QString &url, const QString &formData,
                            bool enabled, bool customUrl)
{
    if (confManager()->addZone(zoneName, sourceCode,
                               url, formData, enabled, customUrl)) {
        reset();
        return true;
    }

    return false;
}

bool ZoneListModel::updateZone(qint64 zoneId, const QString &zoneName,
                               const QString &sourceCode, const QString &url,
                               const QString &formData, bool enabled, bool customUrl,
                               bool updateDriver)
{
    if (confManager()->updateZone(zoneId, zoneName, sourceCode,
                                  url, formData, enabled, customUrl)) {
        refresh();
        return true;
    }

    return false;
}

bool ZoneListModel::updateZoneName(qint64 zoneId, const QString &zoneName)
{
    if (confManager()->updateZoneName(zoneId, zoneName)) {
        refresh();
        return true;
    }

    return false;
}

bool ZoneListModel::updateZoneEnabled(qint64 zoneId, bool enabled)
{
    if (confManager()->updateZoneEnabled(zoneId, enabled)) {
        refresh();
        return true;
    }

    return false;
}

bool ZoneListModel::updateZoneResult(qint64 zoneId, const QString &checksum,
                                     const QDateTime &lastRun,
                                     const QDateTime &lastSuccess)
{
    if (confManager()->updateZoneResult(zoneId, checksum, lastRun, lastSuccess)) {
        refresh();
        return true;
    }

    return false;
}

void ZoneListModel::deleteZone(qint64 zoneId, int row)
{
    beginRemoveRows(QModelIndex(), row, row);

    if (confManager()->deleteZone(zoneId)) {
        invalidateRowCache();
        removeRow(row);
    }

    endRemoveRows();
}

QVariant ZoneListModel::zoneTypeByCode(const QString &typeCode) const
{
    return m_zoneTypesMap.value(typeCode);
}

QVariant ZoneListModel::zoneSourceByCode(const QString &sourceCode) const
{
    return m_zoneSourcesMap.value(sourceCode);
}

bool ZoneListModel::updateTableRow(int row) const
{
    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql().toLatin1(), {row})
          && stmt.step() == SqliteStmt::StepRow))
        return false;

    m_zoneRow.zoneId = stmt.columnInt64(0);
    m_zoneRow.enabled = stmt.columnBool(1);
    m_zoneRow.customUrl = stmt.columnBool(2);
    m_zoneRow.zoneName = stmt.columnText(3);
    m_zoneRow.sourceCode = stmt.columnText(4);
    m_zoneRow.url = stmt.columnText(5);
    m_zoneRow.formData = stmt.columnText(6);
    m_zoneRow.checksum = stmt.columnText(7);
    m_zoneRow.lastRun = stmt.columnDateTime(8);
    m_zoneRow.lastSuccess = stmt.columnDateTime(9);

    return true;
}

QString ZoneListModel::sqlBase() const
{
    return
            "SELECT"
            "    zone_id,"
            "    enabled,"
            "    custom_url,"
            "    name,"
            "    source_code,"
            "    url,"
            "    form_data,"
            "    checksum,"
            "    last_run,"
            "    last_success"
            "  FROM zone"
            ;
}

void ZoneListModel::initZoneTypes()
{
    const auto data = FileUtil::readFileData(":/zone/types.json");
    if (data.isEmpty())
        return;

    QString errorString;
    const auto zoneTypes = JsonUtil::jsonToVariant(data, errorString).toList();
    if (!errorString.isEmpty()) {
        qWarning() << "Zone Types: JSON error:" << errorString;
        return;
    }

    int index = 0;
    for (auto &typeVar : zoneTypes) {
        ZoneTypeWrapper zoneType(typeVar);
        zoneType.setIndex(index++);
        m_zoneTypesMap.insert(zoneType.code(), typeVar);
        m_zoneTypes.append(typeVar);
    }
}

void ZoneListModel::initZoneSources()
{
    const auto data = FileUtil::readFileData(":/zone/sources.json");
    if (data.isEmpty())
        return;

    QString errorString;
    const auto zoneSources = JsonUtil::jsonToVariant(data, errorString).toList();
    if (!errorString.isEmpty()) {
        qWarning() << "Zone Sources: JSON error:" << errorString;
        return;
    }

    int index = 0;
    for (auto &sourceVar : zoneSources) {
        ZoneSourceWrapper zoneSource(sourceVar);
        zoneSource.setIndex(index++);
        m_zoneSourcesMap.insert(zoneSource.code(), sourceVar);
        m_zoneSources.append(sourceVar);
    }
}
