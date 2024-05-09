#ifndef WORKER_TYPES_H
#define WORKER_TYPES_H

#include <QSharedPointer>

class WorkerJob;
class WorkerObject;
class WorkerManager;

using WorkerJobPtr = QSharedPointer<WorkerJob>;

#endif // WORKER_TYPES_H
