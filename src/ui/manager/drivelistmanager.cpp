#include "drivelistmanager.h"

#include <QLoggingCategory>

#include <util/fileutil.h>

namespace {
const QLoggingCategory LC("driveListManager");
}

DriveListManager::DriveListManager(QObject *parent) : QObject(parent) { }

void DriveListManager::initialize()
{
    qCDebug(LC) << "TEST> INIT";
}

void DriveListManager::onDriveListChanged()
{
    m_driveMask = FileUtil::driveMask();

    qCDebug(LC) << "TEST>" << m_driveMask;

    emit driveMaskChanged(m_driveMask);
}
