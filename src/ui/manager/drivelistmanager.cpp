#include "drivelistmanager.h"

#include <QLoggingCategory>
#include <QTimer>

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
    quint32 driveMask = FileUtil::driveMask();
    if (m_checkMounted) {
        driveMask = FileUtil::mountedDriveMask(driveMask);
    }

    if (m_driveMask == driveMask)
        return;

    const quint32 addedMask = (driveMask & (driveMask ^ m_driveMask));
    const quint32 removedMask = (m_driveMask ^ (m_driveMask & m_driveMask));

    m_driveMask = driveMask;

    if (addedMask != 0 || removedMask != 0) {
        emit driveMaskChanged(addedMask, removedMask);
    }
}

void DriveListManager::startPolling()
{
    if (m_checkMounted)
        return;

    m_checkMounted = true;

    setupPollingTimer();

    m_pollingTimer->start();
}

void DriveListManager::setupPollingTimer()
{
    if (m_pollingTimer)
        return;

    m_pollingTimer = new QTimer(this);
    m_pollingTimer->setInterval(1000);

    connect(m_pollingTimer, &QTimer::timeout, this, &DriveListManager::onDriveListChanged);
}
