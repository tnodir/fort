#include "drivelistmanager.h"

#include <QLoggingCategory>

#include <util/fileutil.h>

namespace {
const QLoggingCategory LC("driveListManager");
}

DriveListManager::DriveListManager(QObject *parent) : QObject(parent) { }

void DriveListManager::initialize()
{
    m_driveMask = FileUtil::driveMask();
}

void DriveListManager::onDriveListChanged()
{
    const quint32 driveMask = FileUtil::driveMask();

    if (m_driveMask == driveMask)
        return;

    const quint32 addedMask = (driveMask & (driveMask ^ m_driveMask));

    m_driveMask = driveMask;

    if (addedMask != 0) {
        emit driveMaskAdded(addedMask);
    }
}
