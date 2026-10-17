#include "apppendingconnsearchmodel.h"

#include <algorithm>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitestmt.h>

#include <conf/firewallconf.h>
#include <fortglobal.h>
#include <stat/pendingconnmanager.h>
#include <stat/statsql.h>
#include <util/dateutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>

namespace {

inline constexpr int timerPeriodMsec = 1000;

inline constexpr int connRowsMax = 100;

}

AppPendingConnSearchModel::AppPendingConnSearchModel(QObject *parent) : AppConnSearchModel(parent)
{
    setupTimer();
}

bool AppPendingConnSearchModel::isConnPending(int row) const
{
    return m_pendingConnIds.contains(connRowAt(row).connId);
}

QVariant AppPendingConnSearchModel::data(const QModelIndex &index, int role) const
{
    // The pending connection's timeout or end
    if (index.column() == int(ConnListColumn::Action)) {
        switch (role) {
        case Qt::DecorationRole:
            return actionIcon(index.row());
        case Qt::ToolTipRole:
            return actionToolTip(index.row());
        }
    }

    return AppConnSearchModel::data(index, role);
}

void AppPendingConnSearchModel::updateConnIdRange()
{
    // Append the new pending connections
    QVector<ConnRow> connRows;
    loadConnRows(lastConnId(), connRows);

    m_listedRows.append(connRows);
    appendMatchedConnRows(connRows);

    updatePendingConnIds();

    removeExcessConnRows();

    updateTimer();
    updateTimeoutsProgress();
}

void AppPendingConnSearchModel::clearConnRows()
{
    m_listedRows.clear();

    AppConnSearchModel::clearConnRows();
}

void AppPendingConnSearchModel::updateSearch()
{
    // Another program's connections are loaded
    if (!isListedApp()) {
        setListedApp();

        m_listedRows.clear();

        AppConnSearchModel::updateSearch();
        return;
    }

    applyTextFilter();

    setConnRows(matchedConnRows(m_listedRows));

    updateTimer();
}

StatConnManager *AppPendingConnSearchModel::connManager() const
{
    return Fort::pendingConnManager();
}

QString AppPendingConnSearchModel::sqlSearchWhere() const
{
    return AppConnSearchModel::sqlSearchWhere() + " AND t.conn_time > ?5";
}

void AppPendingConnSearchModel::fillSearchVars(QVariantList &vars) const
{
    AppConnSearchModel::fillSearchVars(vars);

    vars << PendingConnManager::expireTime();
}

void AppPendingConnSearchModel::setupTimer()
{
    m_timer.setInterval(timerPeriodMsec);

    connect(&m_timer, &QTimer::timeout, this, &AppPendingConnSearchModel::updateTimeoutsProgress);
}

void AppPendingConnSearchModel::updateTimer()
{
    if (!hasPendingConns()) {
        m_timer.stop();
    } else if (!m_timer.isActive()) {
        m_timer.start();
    }
}

void AppPendingConnSearchModel::updateTimeoutsProgress()
{
    if (rowCount() == 0)
        return;

    const int column = int(ConnListColumn::Action);

    emit dataChanged(index(0, column), index(rowCount() - 1, column), { Qt::DecorationRole });
}

bool AppPendingConnSearchModel::isListedApp() const
{
    return m_listedConfAppId == confAppId() && m_listedAppPath == appPath();
}

void AppPendingConnSearchModel::setListedApp()
{
    m_listedConfAppId = confAppId();
    m_listedAppPath = appPath();
}

void AppPendingConnSearchModel::updatePendingConnIds()
{
    m_pendingConnIds.clear();

    if (m_listedRows.isEmpty())
        return;

    // The shown rows may be older: the search keeps the ones trimmed from the listed rows
    const qint64 listedIdMin = m_listedRows.constFirst().connId;
    const qint64 connIdMin =
            connRows().isEmpty() ? listedIdMin : qMin(listedIdMin, connRows().constFirst().connId);

    const QVariantList vars = { connIdMin, PendingConnManager::expireTime() };

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(StatSql::sqlSelectPendingConnIds).vars(vars).prepare(stmt))
        return;

    while (stmt.step() == SqliteStmt::StepRow) {
        m_pendingConnIds.insert(stmt.columnInt64(0));
    }
}

void AppPendingConnSearchModel::removeExcessConnRows()
{
    int count = m_listedRows.size() - connRowsMax;

    // The oldest no longer pending ones first
    for (int i = 0; count > 0 && i < m_listedRows.size();) {
        if (m_pendingConnIds.contains(m_listedRows.at(i).connId)) {
            ++i;
        } else {
            removeListedConnRowAt(i);
            --count;
        }
    }

    while (count-- > 0) {
        removeListedConnRowAt(0);
    }
}

void AppPendingConnSearchModel::removeListedConnRowAt(int index)
{
    m_listedRows.removeAt(index);

    // The shown rows are the listed ones, else they're kept till the filter's change
    if (!isFiltering()) {
        removeConnRowsAt(index, /*count=*/1);
    }
}

bool AppPendingConnSearchModel::hasPendingConns() const
{
    const auto &rows = connRows();

    return std::any_of(rows.begin(), rows.end(),
            [&](const ConnRow &connRow) { return m_pendingConnIds.contains(connRow.connId); });
}

QIcon AppPendingConnSearchModel::actionIcon(int row) const
{
    if (!isConnPending(row)) {
        return IconCache::icon(":/icons/help.png");
    }

    // The pending connection's timeout
    return GuiUtil::progressIcon(
            ":/icons/pause.png", timeoutPercent(row), QColor(0xf0, 0xa0, 0x30));
}

QString AppPendingConnSearchModel::actionToolTip(int row) const
{
    if (!isConnPending(row)) {
        return tr("No longer paused");
    }

    return timeoutText(row);
}

qint64 AppPendingConnSearchModel::timeoutSeconds(int row) const
{
    const auto &connRow = connRowAt(row);

    const qint64 timeoutTime =
            connRow.connTime.toSecsSinceEpoch() + Fort::conf().askToConnectSeconds();

    return qMax(timeoutTime - DateUtil::getUnixTime(), qint64(0));
}

int AppPendingConnSearchModel::timeoutPercent(int row) const
{
    const int askSeconds = Fort::conf().askToConnectSeconds();
    if (askSeconds <= 0)
        return 0;

    return int(qMin(timeoutSeconds(row) * 100 / askSeconds, qint64(100)));
}

QString AppPendingConnSearchModel::timeoutText(int row) const
{
    return tr("Blocked in %1 s").arg(timeoutSeconds(row));
}
