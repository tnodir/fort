#include "connsearchmodel.h"

#include <QHash>

#include <algorithm>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitestmt.h>

#include <appinfo/appinfocache.h>
#include <fortglobal.h>
#include <util/fileutil.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>

using namespace Fort;

namespace {

inline constexpr int searchTimerInterval = 150;
inline constexpr int searchConnsMax = 5000;

using AppTextHash = QHash<QString, QString>; // by app path

const QString &appSearchText(AppTextHash &appTexts, const QString &appPath)
{
    QString &text = appTexts[appPath];

    if (text.isEmpty()) {
        text = FileUtil::fileName(appPath) + '\n' + appInfoCache()->appName(appPath);
    }

    return text;
}

QString connRowSearchText(const ConnRow &connRow, const QString &appText)
{
    const bool isIPv6 = connRow.isIPv6;

    const QStringList list = {
        appText,
        QString::number(connRow.pid),
        NetUtil::protocolName(connRow.ipProto),
        NetFormatUtil::ipToText(connRow.localIp, isIPv6),
        NetUtil::serviceName(connRow.localPort),
        QString::number(connRow.localPort),
        NetFormatUtil::ipToText(connRow.remoteIp, isIPv6),
        NetUtil::serviceName(connRow.remotePort),
        QString::number(connRow.remotePort),
        ConnListModel::directionText(connRow.inbound),
        ConnListModel::actionText(connRow.blocked),
        ConnListModel::reasonDetailsText(connRow),
    };

    return list.join('\n');
}

}

ConnSearchModel::ConnSearchModel(QObject *parent) :
    ConnListModel(parent), m_searchTimer(searchTimerInterval)
{
    connect(&m_searchTimer, &QTimer::timeout, this, &ConnSearchModel::updateSearch);
}

void ConnSearchModel::setTextFilter(const QString &filter)
{
    if (m_textFilter == filter)
        return;

    m_textFilter = filter;

    updateSearchLater();
}

void ConnSearchModel::updateSearchLater()
{
    m_searchTimer.startTrigger();
}

void ConnSearchModel::updateConnIdRange()
{
    if (!isFiltering()) {
        ConnListModel::updateConnIdRange();
        return;
    }

    qint64 idMin = 0, idMax = 0;
    fillConnIdRange(idMin, idMax);

    /* Search in the last connections only */
    const qint64 searchIdMin = qMax(idMin, idMax - searchConnsMax + 1);

    removeConnRowsBefore(searchIdMin);

    appendConnRows(qMax(m_lastConnId, searchIdMin - 1));
}

void ConnSearchModel::clearConnRows()
{
    if (!isFiltering()) {
        ConnListModel::clearConnRows();
        return;
    }

    /* The conn_id-s are reused after the table's clearing */
    removeFirstConnRows(m_connRows.size());
    m_lastConnId = 0;
}

bool ConnSearchModel::updateTableRow(const QVariantHash &vars, int row) const
{
    if (!isFiltering())
        return ConnListModel::updateTableRow(vars, row);

    if (!isAscendingOrder()) {
        row = m_connRows.size() - row - 1;
    }

    if (row < 0 || row >= m_connRows.size())
        return false;

    connRow() = m_connRows[row];

    return true;
}

int ConnSearchModel::doSqlCount() const
{
    return isFiltering() ? m_connRows.size() : ConnListModel::doSqlCount();
}

void ConnSearchModel::updateSearch()
{
    m_textMatcher.setFilter(m_textFilter);

    m_connRows.clear();
    m_lastConnId = 0;

    if (isFiltering()) {
        reset();
        updateConnIdRange();
        return;
    }

    qint64 idMin = 0, idMax = 0;
    fillConnIdRange(idMin, idMax);

    resetConnRows(idMin, idMax);
}

void ConnSearchModel::removeConnRowsBefore(qint64 idMin)
{
    const auto it = std::lower_bound(m_connRows.cbegin(), m_connRows.cend(), idMin,
            [](const ConnRow &connRow, qint64 connId) { return connRow.connId < connId; });

    removeFirstConnRows(int(it - m_connRows.cbegin()));
}

void ConnSearchModel::removeFirstConnRows(int count)
{
    if (count <= 0)
        return;

    const int first = isAscendingOrder() ? 0 : (m_connRows.size() - count);

    beginRemoveRows({}, first, first + count - 1);
    m_connRows.remove(0, count);
    invalidateRowCache();
    endRemoveRows();
}

void ConnSearchModel::appendConnRows(qint64 connIdFrom)
{
    QVector<ConnRow> connRows;
    loadConnRows(connIdFrom, connRows);

    const int count = connRows.size();
    if (count == 0)
        return;

    const int first = isAscendingOrder() ? m_connRows.size() : 0;

    beginInsertRows({}, first, first + count - 1);
    m_connRows.append(connRows);
    invalidateRowCache();
    endInsertRows();
}

void ConnSearchModel::loadConnRows(qint64 connIdFrom, QVector<ConnRow> &connRows)
{
    const QString sql = sqlBase() + " WHERE t.conn_id > ?1 ORDER BY t.conn_id LIMIT ?2;";

    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sql).vars({ connIdFrom, searchConnsMax }).prepare(stmt))
        return;

    ConnRow connRow;
    AppTextHash appTexts;

    while (stmt.step() == SqliteStmt::StepRow) {
        fillConnRow(connRow, stmt);

        m_lastConnId = connRow.connId;

        const QString &appText = appSearchText(appTexts, connRow.appPath);

        if (m_textMatcher.isMatched(connRowSearchText(connRow, appText))) {
            connRows.append(connRow);
        }
    }
}
