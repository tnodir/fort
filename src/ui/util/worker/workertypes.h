#ifndef WORKERTYPES_H
#define WORKERTYPES_H

#include <QSharedPointer>

class WorkerJob;
class WorkerObject;
class WorkerManager;

using WorkerJobPtr = QSharedPointer<WorkerJob>;

#endif // WORKERTYPES_H
