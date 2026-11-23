#ifndef ACTIVITYSNAPSHOT_H
#define ACTIVITYSNAPSHOT_H

#include <QHash>
#include <QString>

#include <common/common_types.h>
#include <util/conf/conn.h>

class TextMatcher;

struct ActivityProcRow
{
    quint32 pid = 0;
    quint32 parentPid = 0; // 0 - unknown
    qint64 appId = 0; // 0 - unknown Program
    QString path;
    QString parentPath; // empty - unknown

    bool operator==(const ActivityProcRow &o) const = default;
};

// The Program of the processes: an unknown one by the process's path
struct ActivityAppKey
{
    qint64 appId = 0;
    QString path; // of an unknown Program only

    bool operator==(const ActivityAppKey &o) const = default;
};

inline size_t qHash(const ActivityAppKey &key, size_t seed = 0)
{
    return qHashMulti(seed, key.appId, key.path);
}

struct ActivityTraf
{
    qint64 inBytes = 0;
    qint64 outBytes = 0;
    qint64 inSpeed = 0; // bytes per second
    qint64 outSpeed = 0;

    bool operator==(const ActivityTraf &o) const = default;

    ActivityTraf &operator+=(const ActivityTraf &o)
    {
        inBytes += o.inBytes;
        outBytes += o.outBytes;
        inSpeed += o.inSpeed;
        outSpeed += o.outSpeed;
        return *this;
    }
};

// An allowed flow: its process's path is of the process's row, not in appPath
struct ActivityFlowRow : Conn
{
    quint64 flowId = 0;
    ActivityTraf traf;
};

// The sums of the flows
struct ActivityTotals
{
    int flowCount = 0;
    ActivityTraf traf;

    bool operator==(const ActivityTotals &o) const = default;

    ActivityTotals &operator+=(const ActivityTotals &o)
    {
        flowCount += o.flowCount;
        traf += o.traf;
        return *this;
    }
};

// The Activity DB's rows of one commit
struct ActivitySnapshot
{
    QHash<quint32, ActivityProcRow> procs;
    QHash<quint64, ActivityFlowRow> flows;

    bool read();

    // The programs are matched by their processes' paths
    void removeUnmatchedFlows(const TextMatcher &textMatcher);
};

// A process with flows
struct ActivityShownProc
{
    ActivityAppKey appKey;
    ActivityProcRow row;
    ActivityTotals totals;
};

// The Programs and processes with flows
struct ActivityShown
{
    QHash<ActivityAppKey, ActivityTotals> apps;
    QHash<quint32, ActivityShownProc> procs;

    static ActivityShown build(const ActivitySnapshot &snapshot);

    // Keeps the processes with the snapshot's flows and their Programs
    void removeProcsWithoutFlows(const ActivitySnapshot &snapshot);
};

#endif // ACTIVITYSNAPSHOT_H
