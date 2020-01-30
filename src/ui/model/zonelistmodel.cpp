#include "zonelistmodel.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../conf/confmanager.h"

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
}

int ZoneListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant ZoneListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole: {
            switch (section) {
            case 0: return tr("Zone");
            case 1: return tr("Type");
            case 2: return tr("Last Run");
            case 3: return tr("Last Success");
            }
            break;
        }
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
        case 1: return zoneRow.zoneType;
        case 2: return zoneRow.lastRun;
        case 3: return zoneRow.lastSuccess;
        }

        break;
    }
    }

    return QVariant();
}

const ZoneRow &ZoneListModel::zoneRowAt(int row) const
{
    updateRowCache(row);

    return m_zoneRow;
}

bool ZoneListModel::addZone(const QString &zoneName, const QString &zoneType, const QString &url, const QString &formData, bool enabled)
{
    return false;
}

bool ZoneListModel::updateZone(qint64 zoneId, const QString &zoneName, const QString &zoneType, const QString &url, const QString &formData, bool enabled, bool updateDriver)
{
    return false;
}

void ZoneListModel::deleteZone(qint64 zoneId, int row)
{
}

bool ZoneListModel::updateTableRow(int row) const
{
    SqliteStmt stmt;
    if (!(sqliteDb()->prepare(stmt, sql().toLatin1(), {row})
          && stmt.step() == SqliteStmt::StepRow))
        return false;

    m_zoneRow.zoneId = stmt.columnInt64(0);
    m_zoneRow.enabled = stmt.columnInt(1);
    m_zoneRow.zoneName = stmt.columnText(2);
    m_zoneRow.zoneType = stmt.columnText(3);
    m_zoneRow.url = stmt.columnBool(4);
    m_zoneRow.formData = stmt.columnBool(5);
    m_zoneRow.lastRun = stmt.columnDateTime(6);
    m_zoneRow.lastSuccess = stmt.columnDateTime(7);

    return true;
}

QString ZoneListModel::sqlBase() const
{
    return
            "SELECT"
            "    zone_id,"
            "    enabled,"
            "    name,"
            "    zone_type,"
            "    url,"
            "    form_data,"
            "    last_run,"
            "    last_success"
            "  FROM zone"
            ;
}
