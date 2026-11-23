#include "activitytreemodel.h"

#include <QIcon>

#include <algorithm>
#include <cstring>

#include <ws2tcpip.h>

#include <activity/activitymanager.h>
#include <appinfo/appinfocache.h>
#include <conf/confappmanager.h>
#include <fortglobal.h>
#include <model/applistmodeldata.h>
#include <model/connlistmodel.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/formatutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/net/netformatutil.h>
#include <util/net/netutil.h>

using namespace Fort;

namespace {

bool isFlowChanged(const ActivityFlowRow &a, const ActivityFlowRow &b)
{
    return a.traf != b.traf || a.remoteHost != b.remoteHost;
}

const ActivityAppNode &appNodeOf(const ActivityNode *node)
{
    return *static_cast<const ActivityAppNode *>(node);
}

const ActivityProcNode &procNodeOf(const ActivityNode *node)
{
    return *static_cast<const ActivityProcNode *>(node);
}

const ActivityFlowRow &flowRowOf(const ActivityNode *node)
{
    return static_cast<const ActivityFlowNode *>(node)->row;
}

// Display

// A deleted Program is shown by its process
QString appPathOf(const ActivityAppNode &n)
{
    if (!n.key.path.isEmpty()) {
        return n.key.path;
    }

    return n.children.isEmpty() ? QString() : procNodeOf(n.children.first()).row.path;
}

QString appNameOf(const ActivityAppNode &n)
{
    return n.app.isValid() ? n.app.appName : appInfoCache()->appName(appPathOf(n));
}

QIcon appIconOf(const ActivityAppNode &n)
{
    return n.app.isValid() ? AppListModelData(n.app).appIcon()
                           : appInfoCache()->appIcon(appPathOf(n));
}

QString nameWithPid(const QString &name, quint32 pid)
{
    const QString pidText = QString::number(pid);

    return name.isEmpty() ? pidText : QString("%1 (%2)").arg(name, pidText);
}

// The process of the Program's own exe, not of a wildcard Program's one
bool isAppExeProc(const ActivityProcNode &n)
{
    const auto &appNode = appNodeOf(n.parent);
    const App &app = appNode.app;

    if (!app.isValid()) {
        return n.row.path == appPathOf(appNode);
    }

    return !app.isWildcard && FileUtil::normalizePath(n.row.path) == app.appPath;
}

QString procNameOf(const ActivityProcNode &n)
{
    if (isAppExeProc(n)) {
        return ActivityTreeModel::tr("Process %1").arg(n.row.pid);
    }

    return nameWithPid(appInfoCache()->appName(n.row.path), n.row.pid);
}

QString parentText(const ActivityProcRow &row)
{
    if (row.parentPid == 0)
        return {};

    const QString name =
            row.parentPath.isEmpty() ? QString() : appInfoCache()->appName(row.parentPath);

    return nameWithPid(name, row.parentPid);
}

// The remote host's name or address
QString remoteText(const ActivityFlowRow &f)
{
    if (!f.remoteHost.isEmpty()) {
        return f.remoteHost;
    }

    return NetFormatUtil::ipToAddressText(f.remoteIp, f.isIPv6);
}

QString flowNameOf(const ActivityFlowRow &f)
{
    const QString name = remoteText(f) + ':' + QString::number(f.remotePort);
    if (f.ipProto == IPPROTO_TCP)
        return name;

    return name + " (" + NetUtil::protocolName(f.ipProto) + ')';
}

QVariant procToolTip(const ActivityProcRow &row, int column)
{
    switch (ActivityColumn(column)) {
    case ActivityColumn::Program:
        return row.path;
    case ActivityColumn::Parent:
        return row.parentPath;
    default:
        return {};
    }
}

QString speedText(qint64 bytesPerSecond)
{
    return (bytesPerSecond == 0) ? QString() : FormatUtil::formatSpeed(bytesPerSecond * 8);
}

QString bytesText(qint64 bytes)
{
    return (bytes == 0) ? QString() : FormatUtil::formatDataSize(bytes);
}

// Sort

template<class T>
int compareValues(const T &a, const T &b)
{
    return (a < b) ? -1 : (b < a ? 1 : 0);
}

int compareTexts(const QString &a, const QString &b)
{
    return QString::compare(a, b, Qt::CaseInsensitive);
}

template<ip_addr_t Conn::*field>
int compareIps(const ActivityFlowRow &a, const ActivityFlowRow &b)
{
    if (a.isIPv6 != b.isIPv6) {
        return compareValues(a.isIPv6, b.isIPv6);
    }

    const ip_addr_t &ipA = a.*field;
    const ip_addr_t &ipB = b.*field;

    if (!a.isIPv6) {
        return compareValues(ipA.v4, ipB.v4);
    }

    return std::memcmp(ipA.v6.data, ipB.v6.data, sizeof(ip6_addr_t));
}

int compareAppKeys(const ActivityAppKey &a, const ActivityAppKey &b)
{
    const int res = compareValues(a.appId, b.appId);

    return (res != 0) ? res : compareTexts(a.path, b.path);
}

// The order of the equal rows
int compareNodeIds(const ActivityNode *a, const ActivityNode *b)
{
    switch (a->type) {
    case ActivityNodeType::App:
        return compareAppKeys(appNodeOf(a).key, appNodeOf(b).key);
    case ActivityNodeType::Proc:
        return compareValues(procNodeOf(a).row.pid, procNodeOf(b).row.pid);
    case ActivityNodeType::Flow:
        return compareValues(flowRowOf(a).flowId, flowRowOf(b).flowId);
    default:
        return 0;
    }
}

// Columns: a level without a column's function shows nothing in it and is sorted by the id only

using appDisplay_func = QVariant (*)(const ActivityAppNode &);
using procDisplay_func = QVariant (*)(const ActivityProcNode &);
using flowDisplay_func = QVariant (*)(const ActivityFlowRow &);
using appCompare_func = int (*)(const ActivityAppNode &, const ActivityAppNode &);
using procCompare_func = int (*)(const ActivityProcNode &, const ActivityProcNode &);
using flowCompare_func = int (*)(const ActivityFlowRow &, const ActivityFlowRow &);

struct ActivityColumnFuncs
{
    appDisplay_func appDisplay = nullptr;
    procDisplay_func procDisplay = nullptr;
    flowDisplay_func flowDisplay = nullptr;
    appCompare_func appCompare = nullptr;
    procCompare_func procCompare = nullptr;
    flowCompare_func flowCompare = nullptr;
};

template<class Func, class T>
QVariant callDisplay(Func func, const T &v)
{
    return func ? func(v) : QVariant();
}

template<class Func, class T>
int callCompare(Func func, const T &a, const T &b)
{
    return func ? func(a, b) : 0;
}

const ActivityTraf &trafOf(const ActivityGroupNode &n)
{
    return n.totals.traf;
}

const ActivityTraf &trafOf(const ActivityFlowRow &f)
{
    return f.traf;
}

template<qint64 ActivityTraf::*field, QString (*text)(qint64), class T>
QVariant trafDisplay(const T &v)
{
    return text(trafOf(v).*field);
}

template<qint64 ActivityTraf::*field, class T>
int trafCompare(const T &a, const T &b)
{
    return compareValues(trafOf(a).*field, trafOf(b).*field);
}

// A traffic's column of all the levels
template<qint64 ActivityTraf::*field, QString (*text)(qint64)>
constexpr ActivityColumnFuncs trafColumn()
{
    return {
        .appDisplay = &trafDisplay<field, text, ActivityAppNode>,
        .procDisplay = &trafDisplay<field, text, ActivityProcNode>,
        .flowDisplay = &trafDisplay<field, text, ActivityFlowRow>,
        .appCompare = &trafCompare<field, ActivityAppNode>,
        .procCompare = &trafCompare<field, ActivityProcNode>,
        .flowCompare = &trafCompare<field, ActivityFlowRow>,
    };
}

template<class T>
QVariant flowCountDisplay(const T &n)
{
    return n.totals.flowCount;
}

template<class T>
int flowCountCompare(const T &a, const T &b)
{
    return compareValues(a.totals.flowCount, b.totals.flowCount);
}

const ActivityColumnFuncs columnFuncsList[] = {
    // Program
    {
            .appDisplay = [](const ActivityAppNode &n) -> QVariant { return appNameOf(n); },
            .procDisplay = [](const ActivityProcNode &n) -> QVariant { return procNameOf(n); },
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant { return flowNameOf(f); },
            .appCompare =
                    [](const ActivityAppNode &a, const ActivityAppNode &b) {
                        return compareTexts(appNameOf(a), appNameOf(b));
                    },
            .procCompare =
                    [](const ActivityProcNode &a, const ActivityProcNode &b) {
                        return compareTexts(appInfoCache()->appName(a.row.path),
                                appInfoCache()->appName(b.row.path));
                    },
            .flowCompare =
                    [](const ActivityFlowRow &a, const ActivityFlowRow &b) {
                        const int res = compareTexts(remoteText(a), remoteText(b));

                        return (res != 0) ? res : compareValues(a.remotePort, b.remotePort);
                    },
    },
    // ProcessId
    {
            .procDisplay = [](const ActivityProcNode &n) -> QVariant { return n.row.pid; },
            .procCompare =
                    [](const ActivityProcNode &a, const ActivityProcNode &b) {
                        return compareValues(a.row.pid, b.row.pid);
                    },
    },
    // Parent
    {
            .procDisplay = [](const ActivityProcNode &n) -> QVariant { return parentText(n.row); },
            .procCompare =
                    [](const ActivityProcNode &a, const ActivityProcNode &b) {
                        return compareTexts(parentText(a.row), parentText(b.row));
                    },
    },
    // Protocol
    {
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant {
                return NetUtil::protocolName(f.ipProto);
            },
            .flowCompare =
                    [](const ActivityFlowRow &a, const ActivityFlowRow &b) {
                        return compareValues(a.ipProto, b.ipProto);
                    },
    },
    // LocalAddress
    {
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant {
                return NetFormatUtil::ipToText(f.localIp, f.isIPv6);
            },
            .flowCompare = &compareIps<&ActivityFlowRow::localIp>,
    },
    // LocalPort
    {
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant { return f.localPort; },
            .flowCompare =
                    [](const ActivityFlowRow &a, const ActivityFlowRow &b) {
                        return compareValues(a.localPort, b.localPort);
                    },
    },
    // RemoteAddress
    {
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant {
                return NetFormatUtil::ipToText(f.remoteIp, f.isIPv6);
            },
            .flowCompare = &compareIps<&ActivityFlowRow::remoteIp>,
    },
    // RemotePort
    {
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant { return f.remotePort; },
            .flowCompare =
                    [](const ActivityFlowRow &a, const ActivityFlowRow &b) {
                        return compareValues(a.remotePort, b.remotePort);
                    },
    },
    // HostName
    {
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant { return f.remoteHost; },
            .flowCompare =
                    [](const ActivityFlowRow &a, const ActivityFlowRow &b) {
                        return compareTexts(a.remoteHost, b.remoteHost);
                    },
    },
    trafColumn<&ActivityTraf::inSpeed, speedText>(), // Download
    trafColumn<&ActivityTraf::outSpeed, speedText>(), // Upload
    trafColumn<&ActivityTraf::inBytes, bytesText>(), // Downloaded
    trafColumn<&ActivityTraf::outBytes, bytesText>(), // Uploaded
    // Connections
    {
            .appDisplay = &flowCountDisplay<ActivityAppNode>,
            .procDisplay = &flowCountDisplay<ActivityProcNode>,
            .appCompare = &flowCountCompare<ActivityAppNode>,
            .procCompare = &flowCountCompare<ActivityProcNode>,
    },
    // Time
    {
            .flowDisplay = [](const ActivityFlowRow &f) -> QVariant {
                return DateUtil::formatTime(f.connTime.toSecsSinceEpoch());
            },
            .flowCompare =
                    [](const ActivityFlowRow &a, const ActivityFlowRow &b) {
                        return compareValues(a.connTime, b.connTime);
                    },
    },
};

static_assert(std::size(columnFuncsList) == int(ActivityColumn::Count));

}

ActivityTreeModel::ActivityTreeModel(QObject *parent) :
    QAbstractItemModel(parent), m_root(new ActivityGroupNode(ActivityNodeType::Root))
{
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &ActivityTreeModel::refreshNames);

    connect(confAppManager(), &ConfAppManager::appsChanged, this, &ActivityTreeModel::reloadApps);
    connect(confAppManager(), &ConfAppManager::appUpdated, this, &ActivityTreeModel::reloadApps);
}

ActivityTreeModel::~ActivityTreeModel() = default;

void ActivityTreeModel::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;

    if (active) {
        connect(activityManager(), &ActivityManager::activityChanged, this,
                &ActivityTreeModel::refresh);

        refresh();
    } else {
        disconnect(activityManager(), &ActivityManager::activityChanged, this,
                &ActivityTreeModel::refresh);
    }
}

void ActivityTreeModel::refreshTexts()
{
    emit headerDataChanged(Qt::Horizontal, 0, int(ActivityColumn::Count) - 1);
}

int ActivityTreeModel::columnCount(const QModelIndex & /*parent*/) const
{
    return int(ActivityColumn::Count);
}

int ActivityTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    const ActivityNode *node = parent.isValid() ? nodeOf(parent) : m_root.get();
    if (node->type == ActivityNodeType::Flow)
        return 0;

    return int(static_cast<const ActivityGroupNode *>(node)->children.size());
}

ActivityNode *ActivityTreeModel::nodeOf(const QModelIndex &index)
{
    return static_cast<ActivityNode *>(index.internalPointer());
}

QModelIndex ActivityTreeModel::indexOfNode(const ActivityNode *node, int column) const
{
    if (!node || node == m_root.get())
        return {};

    return createIndex(node->pos, column, const_cast<ActivityNode *>(node));
}

void ActivityTreeModel::emitColumnsChanged(
        const ActivityNode *node, ActivityColumn first, ActivityColumn last)
{
    emit dataChanged(indexOfNode(node, int(first)), indexOfNode(node, int(last)));
}

QModelIndex ActivityTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    const ActivityNode *node = parent.isValid() ? nodeOf(parent) : m_root.get();
    if (node->type == ActivityNodeType::Flow)
        return {};

    const auto *groupNode = static_cast<const ActivityGroupNode *>(node);
    if (row < 0 || row >= groupNode->children.size())
        return {};

    if (column < 0 || column >= int(ActivityColumn::Count))
        return {};

    return createIndex(row, column, groupNode->children.at(row));
}

QModelIndex ActivityTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return {};

    return indexOfNode(nodeOf(child)->parent);
}

QString ActivityTreeModel::columnName(const ActivityColumn column)
{
    const QStringList columnNames = {
        tr("Program"),
        tr("PID"),
        tr("Parent"),
        tr("Protocol"),
        tr("Local Address"),
        tr("Local Port"),
        tr("Remote Address"),
        tr("Remote Port"),
        tr("Host Name"),
        tr("Download"),
        tr("Upload"),
        tr("Downloaded"),
        tr("Uploaded"),
        tr("Connections"),
        tr("Time"),
    };

    return columnNames.value(int(column));
}

QVariant ActivityTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    case Qt::DisplayRole:
        return columnName(ActivityColumn(section));
    case Qt::DecorationRole:
        return headerDataDecoration(section);
    }

    return {};
}

QVariant ActivityTreeModel::headerDataDecoration(int section) const
{
    switch (ActivityColumn(section)) {
    case ActivityColumn::Download:
    case ActivityColumn::Downloaded:
        return IconCache::icon(":/icons/bullet_green_down.png");
    case ActivityColumn::Upload:
    case ActivityColumn::Uploaded:
        return IconCache::icon(":/icons/bullet_blue_up.png");
    }

    return {};
}

QVariant ActivityTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const ActivityNode *node = nodeOf(index);
    const int column = index.column();

    if (role == Qt::DisplayRole) {
        return dataDisplay(node, column);
    }

    if (role == Qt::DecorationRole && column == int(ActivityColumn::Program)) {
        return dataDecoration(node);
    }

    if (role == Qt::FontRole) {
        return dataFont(node, column);
    }

    if (role == Qt::ToolTipRole) {
        return dataToolTip(node, column);
    }

    return {};
}

QVariant ActivityTreeModel::dataDisplay(const ActivityNode *node, int column) const
{
    const auto &funcs = columnFuncsList[column];

    switch (node->type) {
    case ActivityNodeType::App:
        return callDisplay(funcs.appDisplay, appNodeOf(node));
    case ActivityNodeType::Proc:
        return callDisplay(funcs.procDisplay, procNodeOf(node));
    case ActivityNodeType::Flow:
        return callDisplay(funcs.flowDisplay, flowRowOf(node));
    default:
        return {};
    }
}

QVariant ActivityTreeModel::dataDecoration(const ActivityNode *node) const
{
    switch (node->type) {
    case ActivityNodeType::App:
        return appIconOf(appNodeOf(node));
    case ActivityNodeType::Proc:
        return appInfoCache()->appIcon(procNodeOf(node).row.path);
    case ActivityNodeType::Flow:
        return IconCache::icon(ConnListModel::directionBulletIconPath(flowRowOf(node).inbound));
    default:
        return {};
    }
}

QVariant ActivityTreeModel::dataFont(const ActivityNode *node, int column) const
{
    if (node->type == ActivityNodeType::App) {
        return (column == int(ActivityColumn::Program)) ? GuiUtil::fontBold() : QVariant();
    }

    return {};
}

QVariant ActivityTreeModel::dataToolTip(const ActivityNode *node, int column) const
{
    switch (node->type) {
    case ActivityNodeType::Proc:
        return procToolTip(procNodeOf(node).row, column);
    default:
        return {};
    }
}

void ActivityTreeModel::refreshNames()
{
    for (const auto *app : std::as_const(m_apps)) {
        emitColumnsChanged(app, ActivityColumn::Program, ActivityColumn::Program);
    }

    for (const auto *proc : std::as_const(m_procs)) {
        emitColumnsChanged(proc, ActivityColumn::Program, ActivityColumn::Parent);
    }
}

void ActivityTreeModel::reloadApps()
{
    for (auto *node : std::as_const(m_apps)) {
        if (node->key.appId == 0)
            continue;

        node->app = confAppManager()->appById(node->key.appId);

        emitAppNamesChanged(node);
    }
}

void ActivityTreeModel::emitAppNamesChanged(const ActivityAppNode *node)
{
    emitColumnsChanged(node, ActivityColumn::Program, ActivityColumn::Program);

    // A process is named by the Program's path
    for (const auto *proc : node->children) {
        emitColumnsChanged(proc, ActivityColumn::Program, ActivityColumn::Program);
    }
}

// Sorting

int ActivityTreeModel::compareNodes(const ActivityNode *a, const ActivityNode *b) const
{
    if (m_sortColumn < 0 || m_sortColumn >= int(ActivityColumn::Count))
        return 0;

    const auto &funcs = columnFuncsList[m_sortColumn];

    switch (a->type) {
    case ActivityNodeType::App:
        return callCompare(funcs.appCompare, appNodeOf(a), appNodeOf(b));
    case ActivityNodeType::Proc:
        return callCompare(funcs.procCompare, procNodeOf(a), procNodeOf(b));
    case ActivityNodeType::Flow:
        return callCompare(funcs.flowCompare, flowRowOf(a), flowRowOf(b));
    default:
        return 0;
    }
}

bool ActivityTreeModel::lessNode(const ActivityNode *a, const ActivityNode *b) const
{
    const int res = compareNodes(a, b);

    if (res != 0) {
        return (m_sortOrder == Qt::AscendingOrder) ? (res < 0) : (res > 0);
    }

    return compareNodeIds(a, b) < 0;
}

int ActivityTreeModel::childInsertRow(
        const ActivityGroupNode *parent, const ActivityNode *node) const
{
    const auto begin = parent->children.cbegin();

    const auto it = std::lower_bound(begin, parent->children.cend(), node,
            [&](const ActivityNode *a, const ActivityNode *b) { return lessNode(a, b); });

    return int(it - begin);
}

namespace {

void renumberRows(ActivityGroupNode *parent, int from = 0)
{
    for (int i = from; i < parent->children.size(); ++i) {
        parent->children.at(i)->pos = i;
    }
}

void attachNode(ActivityGroupNode *parent, ActivityNode *node, int row)
{
    parent->children.insert(row, node);
    node->parent = parent;

    renumberRows(parent, row);
}

void appendNode(ActivityGroupNode *parent, ActivityNode *node)
{
    attachNode(parent, node, int(parent->children.size()));
}

void detachNode(ActivityNode *node)
{
    ActivityGroupNode *parent = node->parent;
    const int row = node->pos;

    parent->children.removeAt(row);

    renumberRows(parent, row);
}

ActivityAppNode *newAppNode(const ActivityAppKey &key, const ActivityTotals &totals)
{
    auto *node = new ActivityAppNode();
    node->key = key;
    node->totals = totals;

    if (key.appId != 0) {
        node->app = confAppManager()->appById(key.appId);
    }

    return node;
}

ActivityProcNode *newProcNode(const ActivityShownProc &proc)
{
    auto *node = new ActivityProcNode();
    node->row = proc.row;
    node->totals = proc.totals;
    return node;
}

ActivityFlowNode *newFlowNode(const ActivityFlowRow &row)
{
    auto *node = new ActivityFlowNode();
    node->row = row;
    return node;
}

}

void ActivityTreeModel::sortChildren(ActivityGroupNode *parent)
{
    std::stable_sort(parent->children.begin(), parent->children.end(),
            [&](const ActivityNode *a, const ActivityNode *b) { return lessNode(a, b); });

    renumberRows(parent);
}

void ActivityTreeModel::sortAllChildren()
{
    sortChildren(m_root.get());

    for (auto *app : std::as_const(m_apps)) {
        sortChildren(app);
    }

    for (auto *proc : std::as_const(m_procs)) {
        sortChildren(proc);
    }
}

void ActivityTreeModel::sort(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;

    emit layoutAboutToBeChanged();

    const QModelIndexList oldIndexes = persistentIndexList();

    sortAllChildren();

    QModelIndexList newIndexes;
    newIndexes.reserve(oldIndexes.size());

    for (const QModelIndex &oldIndex : oldIndexes) {
        newIndexes.append(oldIndex.isValid() ? indexOfNode(nodeOf(oldIndex), oldIndex.column())
                                             : QModelIndex());
    }

    changePersistentIndexList(oldIndexes, newIndexes);

    emit layoutChanged();
}

// Synchronization with the DB

void ActivityTreeModel::refresh()
{
    ActivitySnapshot snapshot;
    if (!snapshot.read())
        return;

    const ActivityShown shown = ActivityShown::build(snapshot);

    if (m_procs.isEmpty()) {
        resetFromSnapshot(snapshot, shown);
    } else {
        applySnapshot(snapshot, shown);
    }
}

void ActivityTreeModel::clearNodes()
{
    qDeleteAll(m_root->children);
    m_root->children.clear();

    m_apps.clear();
    m_procs.clear();
    m_flows.clear();
}

void ActivityTreeModel::resetFromSnapshot(
        const ActivitySnapshot &snapshot, const ActivityShown &shown)
{
    beginResetModel();

    clearNodes();

    for (auto it = shown.apps.cbegin(); it != shown.apps.cend(); ++it) {
        auto *node = newAppNode(it.key(), *it);
        m_apps.insert(it.key(), node);
        appendNode(m_root.get(), node);
    }

    for (auto it = shown.procs.cbegin(); it != shown.procs.cend(); ++it) {
        auto *node = newProcNode(*it);
        m_procs.insert(it.key(), node);
        appendNode(m_apps.value(it->appKey), node);
    }

    for (const auto &flow : snapshot.flows) {
        auto *node = newFlowNode(flow);
        m_flows.insert(flow.flowId, node);
        appendNode(m_procs.value(flow.pid), node);
    }

    sortAllChildren();

    endResetModel();
}

void ActivityTreeModel::applySnapshot(const ActivitySnapshot &snapshot, const ActivityShown &shown)
{
    removeVanishedNodes(m_flows, snapshot.flows);

    // Their flows are removed: a process is shown by any flow of the snapshot
    removeVanishedNodes(m_procs, shown.procs);

    addNewApps(shown);
    addOrMoveProcs(shown);

    // After the processes' moves
    removeVanishedNodes(m_apps, shown.apps);

    syncTotals(shown);
    syncFlows(snapshot);
}

template<class Key, class Node, class Value>
void ActivityTreeModel::removeVanishedNodes(
        QHash<Key, Node *> &nodes, const QHash<Key, Value> &shownValues)
{
    const QList<Key> keys = nodes.keys();
    for (const Key &key : keys) {
        if (shownValues.contains(key))
            continue;

        removeNode(nodes.take(key));
    }
}

void ActivityTreeModel::addNewApps(const ActivityShown &shown)
{
    for (auto it = shown.apps.cbegin(); it != shown.apps.cend(); ++it) {
        if (m_apps.contains(it.key()))
            continue;

        auto *node = newAppNode(it.key(), *it);
        m_apps.insert(it.key(), node);
        insertNode(m_root.get(), node);
    }
}

void ActivityTreeModel::addOrMoveProcs(const ActivityShown &shown)
{
    for (auto it = shown.procs.cbegin(); it != shown.procs.cend(); ++it) {
        auto *app = m_apps.value(it->appKey);
        auto *node = m_procs.value(it.key());

        if (!node) {
            node = newProcNode(*it);
            m_procs.insert(it.key(), node);
            insertNode(app, node);
            continue;
        }

        // The Program is changed by a new flow's CONN or one logged again
        if (node->parent != app) {
            moveNode(node, app);
        }
    }
}

void ActivityTreeModel::syncTotals(const ActivityShown &shown)
{
    for (auto it = shown.apps.cbegin(); it != shown.apps.cend(); ++it) {
        updateTotals(m_apps.value(it.key()), *it);
    }

    for (auto it = shown.procs.cbegin(); it != shown.procs.cend(); ++it) {
        auto *node = m_procs.value(it.key());

        updateProcRow(node, it->row);
        updateTotals(node, it->totals);
    }
}

void ActivityTreeModel::updateProcRow(ActivityProcNode *node, const ActivityProcRow &row)
{
    if (node->row == row)
        return;

    node->row = row;

    emitColumnsChanged(node, ActivityColumn::Program, ActivityColumn::Parent);
}

void ActivityTreeModel::updateTotals(ActivityGroupNode *node, const ActivityTotals &totals)
{
    if (totals == node->totals)
        return;

    node->totals = totals;

    emitColumnsChanged(node, ActivityColumn::Download, ActivityColumn::Connections);
}

void ActivityTreeModel::syncFlows(const ActivitySnapshot &snapshot)
{
    for (const auto &flow : snapshot.flows) {
        if (auto *node = m_flows.value(flow.flowId)) {
            updateFlow(node, flow);
        } else {
            addFlow(flow);
        }
    }
}

void ActivityTreeModel::updateFlow(ActivityFlowNode *node, const ActivityFlowRow &row)
{
    if (!isFlowChanged(node->row, row))
        return;

    node->row = row;

    // The host name is in the Program's column too
    emitColumnsChanged(node, ActivityColumn::Program, ActivityColumn::Uploaded);
}

void ActivityTreeModel::addFlow(const ActivityFlowRow &row)
{
    auto *node = newFlowNode(row);

    m_flows.insert(row.flowId, node);

    insertNode(m_procs.value(row.pid), node);
}

void ActivityTreeModel::insertNode(ActivityGroupNode *parent, ActivityNode *node)
{
    const int row = childInsertRow(parent, node);

    beginInsertRows(indexOfNode(parent), row, row);

    attachNode(parent, node, row);

    endInsertRows();
}

void ActivityTreeModel::moveNode(ActivityNode *node, ActivityGroupNode *dst)
{
    ActivityGroupNode *src = node->parent;
    const int srcRow = node->pos;
    const int dstRow = childInsertRow(dst, node);

    beginMoveRows(indexOfNode(src), srcRow, srcRow, indexOfNode(dst), dstRow);

    detachNode(node);
    attachNode(dst, node, dstRow);

    endMoveRows();
}

void ActivityTreeModel::removeNode(ActivityNode *node)
{
    const int row = node->pos;

    beginRemoveRows(indexOfNode(node->parent), row, row);

    detachNode(node);

    endRemoveRows();

    delete node;
}
