#ifndef ACTIVITYTREEMODEL_H
#define ACTIVITYTREEMODEL_H

#include <QAbstractItemModel>
#include <QHash>
#include <QList>

#include <memory>

#include <conf/app.h>

#include "activitysnapshot.h"

enum class ActivityColumn : qint8 {
    Program = 0,
    ProcessId,
    Parent,
    Protocol,
    LocalAddress,
    LocalPort,
    RemoteAddress,
    RemotePort,
    HostName,
    Direction,
    Reason,
    DownloadSpeed,
    UploadSpeed,
    DownloadTotal,
    UploadTotal,
    Connections,
    Time,
    Count,
};

enum class ActivityNodeType : qint8 {
    Root = 0,
    App,
    Proc,
    Flow,
};

struct ActivityGroupNode;

struct ActivityNode
{
    explicit ActivityNode(ActivityNodeType type) : type(type) { }
    virtual ~ActivityNode() = default;

    ActivityNodeType type = ActivityNodeType::Root;
    int pos = 0; // the row in the parent
    ActivityGroupNode *parent = nullptr;
};

// The children of one type: the Programs of the root, the processes of a Program, the flows of a
// process
struct ActivityGroupNode : ActivityNode
{
    explicit ActivityGroupNode(ActivityNodeType type) : ActivityNode(type) { }
    ~ActivityGroupNode() override { qDeleteAll(children); }

    ActivityTotals totals;
    QList<ActivityNode *> children;
};

struct ActivityAppNode : ActivityGroupNode
{
    ActivityAppNode() : ActivityGroupNode(ActivityNodeType::App) { }

    ActivityAppKey key;
    App app; // of a known Program, invalid when it's deleted
};

struct ActivityProcNode : ActivityGroupNode
{
    ActivityProcNode() : ActivityGroupNode(ActivityNodeType::Proc) { }

    ActivityProcRow row;
};

struct ActivityFlowNode : ActivityNode
{
    ActivityFlowNode() : ActivityNode(ActivityNodeType::Flow) { }

    ActivityFlowRow row;
};

// The live Programs with their processes and flows, read from the Activity DB. The rows are
// synchronized by the flows' ids, so the expanded state is kept. Sorted only on the header's click.
class ActivityTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ActivityTreeModel(QObject *parent = nullptr);
    ~ActivityTreeModel() override;

    // Reads the DB on its changes only while active
    void setActive(bool active);

    void refreshTexts();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(
            int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    static QString columnName(const ActivityColumn column);

public slots:
    void refresh();

private:
    static ActivityNode *nodeOf(const QModelIndex &index);

    QVariant headerDataDecoration(int section) const;

    QVariant dataDisplay(const ActivityNode *node, int column) const;
    QVariant dataDecoration(const ActivityNode *node, int column) const;
    QVariant dataFont(const ActivityNode *node, int column) const;
    QVariant dataToolTip(const ActivityNode *node, int column) const;

    QModelIndex indexOfNode(const ActivityNode *node, int column = 0) const;

    void emitColumnsChanged(const ActivityNode *node, ActivityColumn first, ActivityColumn last);
    void emitAppNamesChanged(const ActivityAppNode *node);

    void reloadApps();
    void refreshNames();

    int compareNodes(const ActivityNode *a, const ActivityNode *b) const;
    bool lessNode(const ActivityNode *a, const ActivityNode *b) const;
    int childInsertRow(const ActivityGroupNode *parent, const ActivityNode *node) const;

    void sortChildren(ActivityGroupNode *parent);
    void sortAllChildren();

    void resetFromSnapshot(const ActivitySnapshot &snapshot, const ActivityShown &shown);
    void applySnapshot(const ActivitySnapshot &snapshot, const ActivityShown &shown);

    void clearNodes();

    template<class Key, class Node, class Value>
    void removeVanishedNodes(QHash<Key, Node *> &nodes, const QHash<Key, Value> &shownValues);
    void addNewApps(const ActivityShown &shown);
    void addOrMoveProcs(const ActivityShown &shown);
    void syncTotals(const ActivityShown &shown);
    void syncFlows(const ActivitySnapshot &snapshot);

    void insertNode(ActivityGroupNode *parent, ActivityNode *node);
    void moveNode(ActivityNode *node, ActivityGroupNode *dst);
    void removeNode(ActivityNode *node);

    void updateProcRow(ActivityProcNode *node, const ActivityProcRow &row);
    void updateTotals(ActivityGroupNode *node, const ActivityTotals &totals);
    void updateFlow(ActivityFlowNode *node, const ActivityFlowRow &row);
    void addFlow(const ActivityFlowRow &row);

private:
    bool m_active = false;

    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    int m_sortColumn = -1;

    std::unique_ptr<ActivityGroupNode> m_root; // the invisible top

    QHash<ActivityAppKey, ActivityAppNode *> m_apps;
    QHash<quint32, ActivityProcNode *> m_procs;
    QHash<quint64, ActivityFlowNode *> m_flows;
};

#endif // ACTIVITYTREEMODEL_H
