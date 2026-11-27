#ifndef ACTIVITY_TYPES_H
#define ACTIVITY_TYPES_H

#include <QHash>
#include <QString>

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

// A Program's sums of its processes' flows' speeds
struct ActivityAppSpeed
{
    qint64 inSpeed = 0; // bytes per second
    qint64 outSpeed = 0;
    ActivityAppKey appKey;

    bool operator==(const ActivityAppSpeed &o) const = default;
};

#endif // ACTIVITY_TYPES_H
