#include "task.h"

Task::Task(FortManager *fortManager,
           QObject *parent) :
    QObject(parent),
    m_fortManager(fortManager)
{
}
