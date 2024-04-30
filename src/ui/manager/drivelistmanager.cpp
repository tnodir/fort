#include "drivelistmanager.h"

#include <QLoggingCategory>

#include <fortsettings.h>
#include <manager/nativeeventfilter.h>
#include <manager/servicemanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

namespace {
const QLoggingCategory LC("manager.driveList");
}

DriveListManager::DriveListManager(QObject *parent) : QObject(parent) { }

void DriveListManager::setUp()
{
    auto settings = IoC<FortSettings>();

    if (settings->isService()) {
        connect(IoC<ServiceManager>(), &ServiceManager::driveListChanged, this,
                &DriveListManager::onDriveListChanged);
    } else {
        connect(IoC<NativeEventFilter>(), &NativeEventFilter::driveListChanged, this,
                &DriveListManager::onDriveListChanged);
    }

    m_driveMask = FileUtil::driveMask();
}

void DriveListManager::onDriveListChanged()
{
    const quint32 driveMask = FileUtil::driveMask();

    if (m_driveMask == driveMask)
        return;

    const quint32 addedMask = (driveMask & ~m_driveMask);
    const quint32 removedMask = (~driveMask & m_driveMask);

    m_driveMask = driveMask;

    if (addedMask != 0 || removedMask != 0) {
        emit driveMaskChanged(addedMask, removedMask);
    }
}
