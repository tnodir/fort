#include "taskmanager.h"

TaskManager::TaskManager(FortManager *fortManager,
                         QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager)
{
}

QQmlListProperty<Task> TaskManager::tasks()
{
    return QQmlListProperty<Task>(this, m_tasks);
}
